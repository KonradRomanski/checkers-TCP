#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <queue>
#include <map>

#define QUEUE_SIZE 10
#define MSG_SIZE 7
#define BUFF_SIZE 1024
#define board_dim 8

#define lock(sem) pthread_mutex_lock(&sem)
#define unlock(sem) pthread_mutex_unlock(&sem)
#define wait pthread_cond_wait
#define signal pthread_cond_signal
#define broadcast pthread_cond_broadcast

#define protocol_size 7

using namespace std;

int SERVER_PORT = 1111;

struct c_string {
    char str[MSG_SIZE];
};

queue<int> waiting_room;
map<int, int> players;
map<int, c_string> msg;
map<int, bool> turn;

//QUEUE
pthread_cond_t partner = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;

//MAP
pthread_mutex_t mutex_map = PTHREAD_MUTEX_INITIALIZER;

//MSG ARRAY
pthread_mutex_t mutex_msg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_msg = PTHREAD_COND_INITIALIZER;

class Game {
    public:
    // int player1;
    // int player2;
    int board[8][8];
    int the_end;

    Game()
    {
        this->fillUpTheBoard();
    }
    
    private:
    void fillUpTheBoard()
    {
        for (int i = 0; i < board_dim; i++)
            for (int j = 0; j < board_dim; j++)
                this->board[i][j] = -1;
    }

    public:
    void setBlack(int id) //black here
    {
        this->board[2][0] = id;
        this->board[2][2] = id;
        this->board[2][4] = id;
        this->board[2][6] = id;
        this->board[1][1] = id;
        this->board[1][3] = id;
        this->board[1][5] = id;
        this->board[1][7] = id;
        this->board[0][0] = id;
        this->board[0][2] = id;
        this->board[0][4] = id;
        this->board[0][6] = id;
    }
    
    void setWhite(int id) //red here
    {
        this->board[5][1] = id;
        this->board[5][3] = id;
        this->board[5][5] = id;
        this->board[5][7] = id;
        this->board[6][0] = id;
        this->board[6][2] = id;
        this->board[6][4] = id;
        this->board[6][6] = id;
        this->board[7][1] = id;
        this->board[7][3] = id;
        this->board[7][5] = id;
        this->board[7][7] = id;
        
    }
};

map<int, Game*> games;

struct thread_data_t
{
    int connection_socket_descriptor;
    int index;
    char buff[BUFF_SIZE];
    int oponent;
};

//-1 - empty
int board[board_dim][board_dim];

// cvheck if we are inside the board
bool checkBasic(int fx, int fy, int tx, int ty) 
{
    printf("[CHECK] [are we inside the board] %d %d %d %d\n", fx, fy, tx, ty);
    if (fx < 1 || fx > 8) return false;
    if (fy < 1 || fy > 8) return false;
    if (tx < 1 || tx > 8) return false;
    if (ty < 1 || ty > 8) return false;
    printf("[CHECK] [we shouldn't be the same] %d %d %d %d\n", fx, fy, tx, ty);
    if (fx == tx && fy == ty) return false;

    return true;
}

// delete first position, create a second one
void updateArray(int fx, int fy, int tx, int ty, int boa[][board_dim], int player)
{
    boa[fx][fy] = -1;
    boa[tx][ty] = player;
}

int abs(int a)
{
    if (a < 0) 
        return a * (-1);
    return a;
}

// check if a move is proper
bool checkIfProper(string check_this, int id,  int bo[][board_dim]){
    int fromx = check_this[3] - '0';
    int fromy = check_this[4] - '0';
    int tox = check_this[5] - '0';
    int toy = check_this[6] - '0';
    int player = id;

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (bo[x][y] == -1) printf("  "); 
            else printf("%d ", bo[x][y]);
        }
        printf("\n");
    }

    if (checkBasic(fromx, fromy, tox, toy))
    {
        fromx = abs(fromx - 8)%8;
        fromy--;
        tox = abs(tox-8)%8;
        toy--;
        //it must be mine	
        printf("[CHECK] [it must be mine] %d != %d\n", bo[fromx][fromy], player);
        if (bo[fromx][fromy] != player) return false;
        //destination must be empty
        printf("[CHECK] [destination not empty] %d != -1\n", bo[tox][toy]);
        if (bo[tox][toy] != -1) return false;
        //only crossing moves
        printf("[CHECK] [only crossing moves] %d == %d, %d == %d\n", fromx, tox, fromy, toy);
        if (fromx == tox || fromy == toy) return false;

        //if kill, check if there it is an oponent in this place
        printf("[CHECK] [check if player is killed]\n");

        if (abs(fromx-tox) == 2 && abs(fromy-toy) == 2) {
        
            if ((bo[fromx-1][fromy-1] == -1 || bo[fromx-1][fromy-1] == player) && fromx - 2 == tox && fromy - 2 == toy) return false;
            else if ((bo[fromx-1][fromy+1] == -1 || bo[fromx-1][fromy+1] == player) && fromx - 2 == tox && fromy + 2 == toy) return false;
            else if ((bo[fromx+1][fromy-1] == -1 || bo[fromx+1][fromy-1] == player) && fromx + 2 == tox && fromy - 2 == toy) return false;
            else if ((bo[fromx+1][fromy+1] == -1 || bo[fromx+1][fromy+1] == player) && fromx + 2 == tox && fromy + 2 == toy) return false;
            else if (fromx + 2 == tox && fromy + 2 == toy) bo[fromx+1][fromy+1] = -1;
            else if (fromx + 2 == tox && fromy - 2 == toy) bo[fromx+1][fromy-1] = -1;
            else if (fromx - 2 == tox && fromy + 2 == toy) bo[fromx-1][fromy+1] = -1;
            else if (fromx - 2 == tox && fromy - 2 == toy) bo[fromx-1][fromy-1] = -1;
            updateArray(fromx, fromy, tox, toy, bo, player);
            printf("[CHECK] Someone died!!\n");
            return true;
        }
        //if move too far away, deny
        else if (abs(fromx - tox) > 1 || abs(fromy - toy) > 1) {
            printf("[CHECK] [u moved too far] %d > 1, %d > 1\n", abs(fromx - tox), abs(fromy - toy));
            return false;
        }

        updateArray(fromx, fromy, tox, toy, bo, player);
        
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                if (bo[x][y] == -1) printf("  "); 
                else printf("%d ", bo[x][y]);
            }
            printf("\n");
        }

        return true;
    }

    // if (check_this == "9999999") return 0;
    return false;
}

string createID(int sd)
{
    string socket = sd > 9 ? to_string(sd) : "0" + to_string(sd);
    return socket;
}

void *ThreadBehavior(void *t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    int this_index = th_data->connection_socket_descriptor;
    int temp_id = 0;
    th_data->oponent = -9;

    printf("[BORN] [ID %d]\n", this_index);
    turn[this_index] = false;
    // c_string empty_msg, comunicate;
    // strncpy(empty_msg.str, "0000000", MSG_SIZE);
    // lock(mutex_msg);
    // msg[this_index] = empty_msg;
    // unlock(mutex_msg);

    while(1) { 
        while (th_data->oponent == -9) {
                printf("[DEBUG] [ID %d] -9\n", this_index);
                // lock(mutex_queue);
                printf("[DEBUG] [ID %d] Freshly locked\n", this_index);
            if (waiting_room.empty()) {
                printf("[LOG] [ID %d] I'm in\n", this_index);
                turn[this_index] = true;
                games[this_index] = new Game();
                games[this_index]->setWhite(this_index);
                games[this_index]->the_end = 0;
                waiting_room.push(this_index);
                // unlock(mutex_queue);
                printf("[LOG] [ID %d] SLEEP on mutex_queue\n", this_index);
                string identity_beg = createID(th_data->connection_socket_descriptor) + "0";
                const char* ib = identity_beg.c_str();
                write(th_data->connection_socket_descriptor, ib, strlen(ib));

                wait(&partner, &mutex_queue);
                lock(mutex_map);
                if (players.count(this_index) == 1) {
                    th_data->oponent = players[this_index];
                    unlock(mutex_map);
                }
            }
            else /* if (!waiting_room.empty()) */ {
                printf("[DEBUG] [ID %d] Waiting room busy\n", this_index);
                temp_id = waiting_room.front();
                waiting_room.pop();

                // unlock(mutex_queue);
                lock(mutex_map);

                players[temp_id] = this_index;
                th_data->oponent = temp_id;
                players[this_index] = temp_id;
                games[this_index] = games[th_data->oponent];
                games[this_index]->setBlack(this_index);
                games[this_index]->the_end = 0;
                unlock(mutex_map);
                
                string identity_beg = createID(th_data->connection_socket_descriptor) + "1";
                const char* ib = identity_beg.c_str();
                write(th_data->connection_socket_descriptor, ib, strlen(ib)+1);

                broadcast(&partner);
            }

            printf("[LOG] My id: %d, Oponent id: %d\n", this_index, th_data->oponent);
        }


        printf("[INFO] [ID %d]\n", this_index);
        if(turn[this_index]) {
            printf("[LOG] [ID %d] My turn\n", this_index);
            //GET MSG FROM THE FRONTEND
            printf("[LOG] [ID %d] [GET] from watek\n", this_index);
            //id_gracza-ID, kolor_gracza-A, ruch-YYZZ
            //IDAXXYY
            std::string fullmsg = "";
            char buff;
            int if_proper = 1;
            
            while (if_proper = read(th_data->connection_socket_descriptor, &buff, sizeof(char)) > 0) { 
                    if(buff == '\x0a') continue;
                    fullmsg += buff;
                    if (fullmsg.size() == protocol_size) break;
                }
                if (if_proper == 0) 
                {
                    // -> ustaw w klasie, że koniec
                    games[this_index]->the_end = 1;
                    // -> ustaw turę przeciwnika
                    turn[this_index] = false;
                    turn[th_data->oponent] = true;
                    // -> obudź przeciwnika
                    broadcast(&cond_msg);
                }

            while(!checkIfProper(fullmsg, this_index, games[this_index]->board)) {
                fullmsg = "ERROR";
                const char *pack = fullmsg.c_str();
                write(th_data->connection_socket_descriptor, pack, strlen(pack)+1);
                fullmsg = "";
                while (read(th_data->connection_socket_descriptor, &buff, sizeof(char)) > 0) { 
                    if(buff == '\x0a') continue;
                    fullmsg += buff;
                    if (fullmsg.size() == protocol_size) break;
                }

            }

            strncpy(msg[this_index].str, fullmsg.c_str(), MSG_SIZE);
            printf("[LOG] [ID %d] [saving] %s (%ld)\n", this_index, msg[this_index].str, sizeof(msg[this_index].str));
           
            //SEND ACCEPT TO THE FRONTEND        
            fullmsg = "ACCEPT";
            const char *pack = fullmsg.c_str();
            write(th_data->connection_socket_descriptor, pack, strlen(pack));
            fullmsg = "";
            turn[this_index] = false;
            turn[th_data->oponent] = true;
            broadcast(&cond_msg);
        }
        else {
            while(!turn[this_index]) {
                printf("[LOG] [ID %d] Not my turn", this_index);
                wait(&cond_msg, &mutex_msg);
            }
            
            // -> jeżeli w klasie ustawiono koniec gry
            if (games[this_index]->the_end) 
            {
            // -> zamknij socket
                    close(th_data->connection_socket_descriptor);
            // (posprzątaj??)
                    free(t_data);
            // zakończ wątek
                    pthread_exit(NULL);
            }

            //SEND MSG TO THE FRONTEND
            printf("[LOG] [ID %d] [SEND] oponent move \n", this_index);
            // strncpy(th_data->buff, msg[th_data->oponent].str, BUFF_SIZE);
            const char* enemy_move = msg[th_data->oponent].str;
            printf("[LOG] [ID %d] [sending] %s\n", this_index, enemy_move);
            write(th_data->connection_socket_descriptor, enemy_move, sizeof(enemy_move)+1);
            unlock(mutex_msg);
        }

    }

    // -> ustaw w klasie, że koniec
    games[this_index]->the_end = 1;
    // -> ustaw turę przeciwnika
    turn[this_index] = false;
    turn[th_data->oponent] = true;
    // -> obudź przeciwnika
    broadcast(&cond_msg);

    printf(" >>>> posprzątaj\n");

    // -> zamknij socket
    close(th_data->connection_socket_descriptor);
    free(t_data);
    pthread_exit(NULL);
}


void handleConnection(int connection_socket_descriptor) {
    int create_result = 0;

    pthread_t thread1;

    struct thread_data_t* t_data = (struct thread_data_t*)malloc(sizeof(struct thread_data_t));
    t_data->connection_socket_descriptor = connection_socket_descriptor;


    create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
       printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(-1);
    }

}


int main(int argc, char* argv[])
{
   int server_socket_descriptor;
   int connection_socket_descriptor;
   int bind_result;
   int listen_result;
   char reuse_addr_val = 1;
   struct sockaddr_in server_address;

//    if (argc == 1) {
//    }
       SERVER_PORT = atoi(argv[1]);
    
   printf("[INFO] port serwera ustawiony na: %d", SERVER_PORT);

   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   server_address.sin_addr.s_addr = htonl(INADDR_ANY);
   server_address.sin_port = htons(SERVER_PORT);

   server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket_descriptor < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
       exit(1);
   }
   setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

   bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (bind_result < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
       exit(1);
   }

   listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
   if (listen_result < 0) {
       fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
       exit(1);
   }

   while(1)
   {
       connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
       if (connection_socket_descriptor < 0)
       {
           fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
           exit(1);
       }

       handleConnection(connection_socket_descriptor);
   }

   close(server_socket_descriptor);
   return(0);
}
