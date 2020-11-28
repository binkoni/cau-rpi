const PORT = 8080
const express = require('express');
const app = express();
const http = require('http').createServer(app);
const spawn = require('child_process').spawn;
const serialport = require("serialport");
const sp = new serialport('/dev/ttyACM0', {baudRate: 9600});

app.use(express.urlencoded({extended: true}));
app.use('/', express.static(__dirname + '/static'));

app.get('/capture', (req, res) => {
  const capture = spawn('../capture');
  capture.on('exit', () => res.redirect('/'));
});

app.post('/7segment', (req, res) => {
  sp.write('7SEG:' + req.body.val);
  res.redirect('/');
});

app.post('/lcd', (req, res) => {
  sp.write('LCD:' + req.body.val);
  res.redirect('/');
});

http.listen(PORT, () => {
  console.log(`listening ${PORT}`);
});


