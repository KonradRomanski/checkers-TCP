var express = require('express');
var bodyParser = require('body-parser');
var app = express();

const net = require('net');

const appPort = process.argv[2] || 8000;


const PORT = process.argv[3] || 1112;
const HOST = 'localhost';

const socket = new net.Socket();

let enemyMove = '';
let self = '';

socket.connect(PORT, HOST, () => {
    console.log(`[LOG] [TCP] Listening on ${HOST}:${PORT}`);
})

app.use(express.static('files'));
app.use(bodyParser.json())

app.get('/', function (req, res) {
    res.sendFile(__dirname + "/" + "index.html");
})


app.get('/enemy', (req, res) => {
    res.send(enemyMove);
    enemyMove = ''
})


app.get('/whoami', (req, res) => {
    const color = self.slice(2) == 0 ? 'black' : 'red';
    console.log({color});
    res.send({color});
})

let lastMoveStatus = ''

app.get('/status', (req, res) => {
    console.log(`[GET] status: ${lastMoveStatus}`);
    res.send({status: lastMoveStatus})
    lastMoveStatus = ''
})

app.post('/move', function (req, res) {
    console.log(req.body);
    socket.write("abc" + req.body.from + req.body.to)
    res.send('posted');
    // res.send(Math.random() > 0.5 ? 'ACCEPT' : 'ERROR');
})

var server = app.listen(appPort, function () {
    var host = server.address().address;
    var port = server.address().port;
    console.log(`[LOG] [HTTP] Listening at ${host}:${port}`);
});

socket.on('error', (e) => {
    console.error(e)
})

socket.on('data', (buffer) => {
    const message = buffer.toString();
    console.log(`[LOG] [MESSAGE] ${message}`);
    if (self === '') {
        self = message.slice(0,3);
        console.log(`[LOG] I am ${self}`);
    }
    if (message.includes('ACCEPT')) {
        lastMoveStatus = 'ACCEPT'
    }
    else if (message.includes('ERROR'))
    {
        lastMoveStatus = 'ERROR'
    }
    else {
        const str = buffer.toString()
        const move = { from: str.slice(3, 5), to: str.slice(5,7) }
        console.log({move});
        enemyMove = move
    }
})

// todo: jeżeli zamknięto socket
// ustaw status dla kienta
