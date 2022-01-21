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

#define SERVER_PORT 1112
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
    void setWhite(int id) //black here
    {
        this->board[2][1] = id;
        this->board[2][3] = id;
        this->board[2][5] = id;
        this->board[2][7] = id;
        this->board[1][0] = id;
        this->board[1][2] = id;
        this->board[1][4] = id;
        this->board[1][6] = id;
        this->board[0][1] = id;
        this->board[0][3] = id;
        this->board[0][5] = id;
        this->board[0][7] = id;
    }
    
    void setBlack(int id) //red here
    {
        this->board[5][0] = id;
        this->board[5][2] = id;
        this->board[5][4] = id;
        this->board[5][6] = id;
        this->board[6][1] = id;
        this->board[6][3] = id;
        this->board[6][5] = id;
        this->board[6][7] = id;
        this->board[7][0] = id;
        this->board[7][2] = id;
        this->board[7][4] = id;
        this->board[7][8] = id;
        
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

bool checkBasic(int fx, int fy, int tx, int ty) 
{
    //basic check here
}

void updateArray(int fx, int fy, int tx, int ty, &boa)
{
    //update array here
}

bool checkIfProper(string check_this, int *bo){
    int fromx = check_this[3] - '0';
    int fromy = check_this[4] - '0';
    int tox = check_this[5] - '0';
    int toy = check_this[6] - '0';

    if (checkBasic(fromx, fromy, tox, toy))
    {
        //check with pawns here
        updateArray(fromx, fromy, tox, toy, &bo)
        return 1;
    }

    if (check_this == "9999999") return 0;
    return 0;
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
    c_string empty_msg, comunicate;
    strncpy(empty_msg.str, "0000000", MSG_SIZE);
    lock(mutex_msg);
    msg[this_index] = empty_msg;
    unlock(mutex_msg);

    while(1) { 
        printf("[AGAIN] [ID %d]\n", this_index);
        // bzero(th_data->buff, BUFF_SIZE);
        printf("[LOG] [ID %d] Oponent: %d, Empty waiting room: %d\n", this_index, th_data->oponent, waiting_room.empty());
        while (th_data->oponent == -9) {
                printf("[DEBUG] [ID %d] -9\n", this_index);
                // lock(mutex_queue);
                printf("[DEBUG] [ID %d] Freshly locked\n", this_index);
            if (waiting_room.empty()) {
                printf("[LOG] [ID %d] I'm in\n", this_index);
                turn[this_index] = true;
                games[this_index] = new Game();
                games[this_index].setWhite(this_index);
                waiting_room.push(this_index);
                // unlock(mutex_queue);
                printf("[LOG] [ID %d] SLEEP on mutex_queue\n", this_index);
                string identity_beg = "i" + to_string(th_data->connection_socket_descriptor) + "0";
                const char* ib = identity_beg.c_str();
                write(th_data->connection_socket_descriptor, ib, strlen(ib));

                const char welcome[8] = { '0', '0', '0', '0', '0', '0', '0', '\0' };
                write(th_data->connection_socket_descriptor, &welcome, 8);
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
                games[this_index] = &games[th_data->oponent];
                games[this_index].setBlack(this_index);

                unlock(mutex_map);
                
                string identity_beg = "i" + to_string(th_data->connection_socket_descriptor) + "1";
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
            
            while (read(th_data->connection_socket_descriptor, &buff, sizeof(char)) > 0) { 
                    if(buff == '\x0a') continue;
                    fullmsg += buff;
                    if (fullmsg.size() == protocol_size) break;
                }

            while(!checkIfProper(fullmsg, &games[this_index])) {
                fullmsg = "ERROR";
                const char *pack = fullmsg.c_str();
                write(th_data->connection_socket_descriptor, pack, strlen(pack)+1);
                fullmsg = "";
                while (read(th_data->connection_socket_descriptor, &buff, sizeof(char)) > 0) { 
                    if(buff == '\x0a') continue;
                    fullmsg += buff;
                    if (fullmsg.size() == protocol_size) break;
                }

                break;
            }
            // strncpy(comunicate.str, th_data->buff, BUFF_SIZE);
            // lock(mutex_msg);
            // strncpy(comunicate.str, "MSG", MSG_SIZE);
            strncpy(msg[this_index].str, fullmsg.c_str(), MSG_SIZE);
            printf("[LOG] [ID %d] [saving] %s (%ld)\n", this_index, msg[this_index].str, sizeof(msg[this_index].str));
            // msg[this_index] = comunicate;
            // unlock(mutex_msg);
            //SEND ACCEPT TO THE FRONTEND
                        
            fullmsg = "ACCEPT";
            const char *pack = fullmsg.c_str();
            write(th_data->connection_socket_descriptor, pack, strlen(pack));
            fullmsg = "";
            turn[this_index] = false;
            turn[th_data->oponent] = true;
            games
            broadcast(&cond_msg);
        }
        else {
            while(!turn[this_index]) {
                printf("[LOG] [ID %d] Not my turn", this_index);
                wait(&cond_msg, &mutex_msg);
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