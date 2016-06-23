var serialPort = require('serialport');
var bufferEqual = require('buffer-equal');

var portNames = {
    host: '/dev/cu.usbserial-DB00JAKZ',
    device: '/dev/cu.usbserial-DB00J5PF'
}
var stream = null;
var sampleCounter = 0;
var newSample = (num) => {
    if (num > 255) {
        num = 0;
    }
    return new Buffer([0xA0, num, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xC0]);
}

var testPacketsToSend = 10;
var packetsRecieved = 0;
var lastPacketNumber = 0;
var badPackets = 0;
var goodPackets = 0;

var deviceSerial, hostSerial;

hostSerial = new serialPort.SerialPort(portNames.host, {
    baudRate: 115200
},(err) => {
    if (err) reject(err);
});

hostSerial.on('data',(data) => {
    // Validate
    if (bufferEqual(data,newSample([data[1]]))) {
        goodPackets++;
    } else {
        badPackets++;
    }
    if(data[1] > testPacketsToSend) {
        console.log(`Good Packets: ${goodPackets} | Bad Packets: ${badPackets}`);

        process.exit();
    }
    console.log('data',data);
});

hostSerial.on('open',() => {
    console.log('host serial open');

});

hostSerial.on('close',() => {
    console.log('host serial close');

});

hostSerial.on('error',(err) => {
    console.log('host serial err',err);

});

deviceSerial = new serialPort.SerialPort(portNames.device, {
    baudRate: 115200
},(err) => {
    if (err) reject(err);
});

deviceSerial.on('data',(data) => {
    console.log('device serial data',data);

});

deviceSerial.on('open',() => {
    console.log('device serial open');

    stream = setInterval(() => {
        deviceSerial.write(newSample(sampleCounter));
        sampleCounter++;
    }, 500);
});

deviceSerial.on('close',() => {
    console.log('device serial close');

});

deviceSerial.on('error',(err) => {
    console.log('device serial error',err);

});

process.on('exit', (code) => {
  if (deviceSerial) deviceSerial.close();
  if (hostSerial) hostSerial.close();
  if (stream) stream = null;
});
