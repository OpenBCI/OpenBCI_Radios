var bufferEqual = require('buffer-equal');
var StreamSearch = require('streamsearch');
var chai = require('chai')
    ,  expect = chai.expect
    ,  should = chai.should();
var OpenBCIBoard = require('openbci').OpenBCIBoard;
var ourBoard = new OpenBCIBoard({verbose:true, baudRate:921600});

var portNames = {
    host: '/dev/tty.usbserial-DB00JAKZ'
}

var badPackets = 0;
var goodPackets = 0;
var rawSampleCount = 0;
var sampleRecievedCounter = 0;

var startHost = () => {
    ourBoard.connect(portNames.host).then(() => {
        ourBoard.on('ready',function() {
            console.log(`YOOOOO`);
            ourBoard.streamStart();
                // ourBoard.radioBaudRateSet('fast')
                //     .then(() => {
                //
                //     })

                // ourBoard.syncClocksStart().catch(err => console.log('sync err',err));
            });
        // ourBoard.on('synced',() => {
        //     process.exit();
        //
        // })
        // ourBoard.on('rawDataPacket', rawDataPacket => {
        //     console.log('rawDataPacket',rawDataPacket);
        // })
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
                if (badPackets >= 5) {
                    ourBoard.streamStop()
                        .then(() => {
                            return ourBoard.disconnect()
                        })
                        .then(() => {
                            process.exit();
                        })
                        .catch(err => {
                            process.exit(1);
                        })

                }
                sampleRecievedCounter = sample.sampleNumber + 1;
            }

            // Loop the counter back to 0
            if (sampleRecievedCounter > 255) sampleRecievedCounter = 0;

        });
    }).catch(err => {
        console.log(`connect error ${err}`);
        process.exit();
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
