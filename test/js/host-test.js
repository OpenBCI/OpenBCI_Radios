var serialPort = require('serialport');
var bufferEqual = require('buffer-equal');
var StreamSearch = require('streamsearch');
var chai = require('chai')
    ,  expect = chai.expect
    ,  should = chai.should();
var OpenBCIBoard = require('openbci').OpenBCIBoard;
var ourBoard = new OpenBCIBoard({verbose:true});

var portNames = {
    host: '/dev/cu.usbserial-DB00JAKZ'
}

var badPackets = 0;
var goodPackets = 0;
var rawSampleCount = 0;
var sampleRecievedCounter = 0;

var startHost = () => {
    ourBoard.connect(portNames.host).then(() => {
        ourBoard.on('ready',function() {
                // ourBoard.streamStart();
                ourBoard.syncClocksStart().catch(err => console.log('sync err',err));
            });
        ourBoard.on('synced',() => {
            process.exit();

        })
        ourBoard.on('rawDataPacket', rawDataPacket => {
            console.log('rawDataPacket',rawDataPacket);
        })
        ourBoard.on('sample',function(sample) {
            rawSampleCount++;
            // console.log(`got sample ${sample.sampleNumber} expecting ${sampleRecievedCounter}`);
            if (sample.timestamp) {
                console.log(sample.timestamp);
            }
            if (sample.sampleNumber === sampleRecievedCounter) {
                // console.log(`\tgood`);
                goodPackets++;
                sampleRecievedCounter++;
            } else {
                console.log(`err: expected ${sampleRecievedCounter} got ${sample.sampleNumber} `);
                badPackets++;
                if (badPackets > 5) {
                    process.exit();
                }
                sampleRecievedCounter = sample.sampleNumber + 1;
            }

            // Loop the counter back to 0
            if (sampleRecievedCounter > 255) sampleRecievedCounter = 0;

        });
    })
}

startHost();

process.on('exit', (code) => {
    console.log(`Total Packets: ${rawSampleCount}\n\tGood Packets: ${goodPackets}\n\tBad Packets: ${badPackets}`);
    if (ourBoard) {
        if (ourBoard.connected) {
            ourBoard.disconnect().then(() => {

            });
        }
    }
});
