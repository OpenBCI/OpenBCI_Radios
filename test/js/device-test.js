var serialPort = require('serialport');
var bufferEqual = require('buffer-equal');

var portNames = {
    device: '/dev/tty.usbserial-DB00J8E9'
}
var stream = null;
var sampleCounter = 0;
var newSample = (num) => {
    if (num > 255) {
        num = 0;
    }
    return new Buffer([0x41, num, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xC0]);
}

var testPacketsToSend = 10;
var packetsRecieved = 0;
var lastPacketNumber = 0;
var badPackets = 0;
var goodPackets = 0;
var sampleRate = 50;
var packetIntervalMS = 1 / sampleRate * 1000;

var deviceSerial;

deviceSerial = new serialPort.SerialPort(portNames.device, {
    baudRate: 115200
},(err) => {
    if (err) {
        console.log(`error opening serialport: ${err}`);
    }
});



var stopStream = () => {
    console.log('stream stopped');
    if (stream) {
        clearInterval(stream);
        stream = null;
    }
    if (deviceSerial) deviceSerial.close();
}

var startStream = () => {
    console.log('stream started');
    stream = setInterval(() => {
        if (sampleCounter >= testPacketsToSend) {
            stopStream();
        } else {
            deviceSerial.write(newSample(sampleCounter));
            sampleCounter++;
        }
    }, packetIntervalMS);
}

deviceSerial.on('data',(data) => {
    console.log('device serial data:',data);

});

deviceSerial.on('open',() => {
    console.log('device serial open');


    sampleCounter = 0;
    startStream();
});

deviceSerial.on('close',() => {
    console.log('device serial close');

});

deviceSerial.on('error',(err) => {
    console.log('device serial error',err);

});

process.on('exit', (code) => {
  if (deviceSerial) {
      if (deviceSerial.isOpen()) {
          deviceSerial.close();
      }
  }
  if (stream) stream = null;
});
