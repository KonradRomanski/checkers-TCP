const net = require('net');
const {Input} = require('enquirer');

const PORT = 1111;
const HOST = 'localhost';

const socket = new net.Socket();

socket.connect(PORT, HOST, () => {
    console.log(`Listening on ${HOST}:${PORT}`);
})

socket.on('data', (buffer) => {
    const message = buffer.toString();
    if (message === 'ACCEPT') {
        console.log('Accepted!')
        return;
    }
    if (message === '0000000\0') {
        console.log('You start!')
    }
    else console.log(`Data:`, buffer, buffer.toString());
    new Input({ message: 'What is your message to the world?' })
        .run()
        .then(v => {
            if (v === 'exit') { socket.destroy(); return; }
            socket.write(v)
        })
        .catch(() => {socket.destroy()})
})