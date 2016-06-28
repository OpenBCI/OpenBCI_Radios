var serialPort = require('serialport');
var bufferEqual = require('buffer-equal');
var StreamSearch = require('streamsearch');
var chai = require('chai')
    ,  expect = chai.expect
    ,  should = chai.should();
var OpenBCIBoard = require('openbci').OpenBCIBoard;
var ourBoard = new OpenBCIBoard({verbose:true});

var portNames = {
    host: '/dev/cu.usbserial-DB00JAKZ',
    device: '/dev/tty.usbserial-DB00J8E9'
}

var stream = null;
var sampleCounter = 0;
var deviceSample = (num) => {
    if (num > 255) {
        num = 0;
    }
    return new Buffer([0x41, num, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xC0]);
}

var hostSample = (num) => {
    if (num > 255) {
        num = 0;
    }
    return new Buffer([0xA0, num, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xC0]);
}

var testPacketsToSend = 2000;
var packetsRecieved = 0;
var lastPacketNumber = 0;
var sampleRecievedCounter = 0;
var badPackets = 0;
var goodPackets = 0;
var sampleRate = 250;
var packetIntervalMS = 1 / sampleRate * 1000;
var deviceBaudRate = 115200;

var timeoutDuration = 3000 + (testPacketsToSend * packetIntervalMS); // 10 seconds

var deviceSerial;

var stopStream = () => {
    // console.log('stream stopped');
    if (stream) {
        clearInterval(stream);
        stream = null;
    }
}
var rawSampleCountSent = 0;
var startStream = () => {
    // console.log('stream started');
    stream = setInterval(() => {
        if (rawSampleCountSent < testPacketsToSend) {
            // console.log(`sending ${sampleCounter}`);
            deviceSerial.write(deviceSample(sampleCounter));
            sampleCounter++;
            if (sampleCounter > 255) sampleCounter = 0;
            rawSampleCountSent++;
        }
    }, packetIntervalMS);
}

var streamStartTimeout = () => {
    return setTimeout(() => {
        console.log(`timeout after ${timeoutDuration/1000} seconds`);
        safeStop();
    }, timeoutDuration);
}
var safeStop = () => {
    console.log(`Total Packets: ${rawSampleCount}\n\tGood Packets: ${goodPackets}\n\tBad Packets: ${badPackets}`);
    ourBoard.disconnect().then(() => {
        setTimeout(() => {
            process.exit();
        }, 400);
    });
}
var sampleRecievedCounter = 0;
var done = false;
var rawSampleCount = 0;
var startHost = () => {
    ourBoard.connect(portNames.host).then(() => {
        ourBoard.on('ready',function() {
                ourBoard.streamStart();
                streamStartTimeout();
            });
        // ourBoard.on('rawDataPacket', rawDataPacket => {
        //     console.log('rawDataPacket',rawDataPacket);
        // })
        ourBoard.on('sample',function(sample) {
            rawSampleCount++;
            /** Work with sample */
            if (done) return;
            // console.log(`got sample ${sample.sampleNumber} expecting ${sampleRecievedCounter}`);

            if (sample.sampleNumber === sampleRecievedCounter) {
                // console.log(`\tgood`);
                goodPackets++;
                sampleRecievedCounter++;
            } else {
                console.log(`err: expected ${sampleRecievedCounter} got ${sample.sampleNumber} `);
                badPackets++;
                sampleRecievedCounter = sample.sampleNumber + 1;
            }

            // Loop the counter back to 0
            if (sampleRecievedCounter > 255) sampleRecievedCounter = 0;

            if(rawSampleCount + 1 > testPacketsToSend) {
                done = true;
                safeStop();
                clearInterval(stream);
            }
        });
    })
}


deviceSerial = new serialPort.SerialPort(portNames.device, {
    baudRate: deviceBaudRate
},(err) => {
    if (err) {
        console.log(`error opening device serialport: ${err}`);
        process.exit(2);
    } else {

    }
});
deviceSerial.on('data',(data) => {
    // console.log('device serial data:',data.toString());
    if (doesHaveSoftResetCmd(data)) {
        sendFakeSoftResetMsg();
    } else if (doesHaveStart(data)) {
        startStream();
    } else if (doesHaveStop(data)) {
        stopStream();
    } else {
        console.log(`device data:`, data);
    }

});

deviceSerial.on('open',() => {
    console.log('device serial open');
    startHost(); // will send `v` then `s`
});

deviceSerial.on('close',() => {
    console.log('device serial close');
});

deviceSerial.on('error',(err) => {
    console.log('device serial error',err);
});

var doesHaveSoftResetCmd = function(dataBuffer) {
    const s = new StreamSearch(new Buffer("v"));

    // Clear the buffer
    s.reset();

    // Push the new data buffer. This runs the search.
    s.push(dataBuffer);

    // Check and see if there is a match
    return s.matches;
};

var doesHaveStart = function(dataBuffer) {
    const s = new StreamSearch(new Buffer("b"));

    // Clear the buffer
    s.reset();

    // Push the new data buffer. This runs the search.
    s.push(dataBuffer);

    // Check and see if there is a match
    return s.matches;
};

var doesHaveStop = function(dataBuffer) {
    const s = new StreamSearch(new Buffer("s"));

    // Clear the buffer
    s.reset();

    // Push the new data buffer. This runs the search.
    s.push(dataBuffer);

    // Check and see if there is a match
    return s.matches;
};

var sendFakeSoftResetMsg = () => {
    console.log('device sending fake soft reset');
    deviceSerial.write(new Buffer(`OpenBCI V3 Simulator\nOn Board ADS1299 Device ID: 0x12345\nLIS3DH Device ID: 0x38422$$$`));
    deviceSerial.flush();
};

process.on('exit', (code) => {
    if (deviceSerial) {
        if (deviceSerial.isOpen()) {
            deviceSerial.close();
        }
    }
    if (ourBoard) {
        if (ourBoard.connected) {
            ourBoard.disconnect().then(() => {

            });
        }
    }
    if (stream) {
        clearInterval(stream);
        stream = null;
    }
});
