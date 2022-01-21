var express = require('express');
var bodyParser = require('body-parser');
var app = express();

const net = require('net');

const PORT = 1112;
const HOST = 'localhost';

const socket = new net.Socket();

let enemyMove = ''

socket.connect(PORT, HOST, () => {
    console.log(`Listening on ${HOST}:${PORT}`);
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

app.post('/move', function (req, res) {
    console.log(req.body);
    res.send('ACCEPT');
    socket.write("abc" + req.body.from + req.body.to)
    res.send(Math.random() > 0.5 ? 'ACCEPT' : 'ERROR');
})

var server = app.listen(8000, function () {
    var host = server.address().address;
    var port = server.address().port;
    console.log('Listening at http://%s:%s', host, port);
});


socket.on('data', (buffer) => {
    const message = buffer.toString();
    if (message === 'ACCEPT') {
        console.log('Accepted!')
        return;
    }
    if (message === '0000000\0') {
        console.log('You start!')
    }
    else {
        console.log(`Data:`, buffer, buffer.toString());
        const str = buffer.toString()
        const move = { from: str.slice(3, 5), to: str.slice(5,7) }
        console.log({move});
        enemyMove = move
    }
})