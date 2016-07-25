var OpenBCIBoard = require('openbci').OpenBCIBoard,
    ourBoard = new OpenBCIBoard({
        verbose:true
    }),
    fs = require('fs'),
    wstreamSample = fs.createWriteStream('Hardware_timestamp-samples1.csv'),
    util = require('util'),
    cp = require('child_process'),
    robot = require("robotjs");

var child;

var portNames = {
    host: '/dev/cu.usbserial-DB00MHK7'
}

var writeHeaderDataLog = () => {
    wstreamSample.write(`Time Stamp,Digital Value,Board Time\n`);
}

const sampleRate = 250;
var runTimeSec = 10; //seconds
var totalSamplesToGet = sampleRate * runTimeSec;
var rawSampleCount = 0;
var sampleRecievedCounter = 0;
var timeSentHighArray = [];
var timeSentLowArray = [];
var lastTimeSent = 0;
var cycleTimeMS = 500;
var isCycleHigh = false;
var cycleCount = 0;
var startLoggingSamples = false;


var endKindly = () => {
    if (ourBoard.connected) {
        ourBoard.disconnect()
            .then(() => {
                process.exit();
            })
            .catch(err => {
                $log.error(err);
                process.exit();
            });
    } else {
        process.exit();
    }
}

var startHardwareFlasher = () => {
    child = cp.fork(`test/js/radio-dongle-time-sync-test.js`);
    child.on('message', m => {
        console.log('PARENT got msg:',m);
        if (m.body === "childDone") {
            endKindly();
        }
    });
    child.send({body:"start"});
    setTimeout(() => {
        child.send({body:"stop"});
        for ( var i = 0; i < samples.length; i++) {
            wstreamSample.write(`${samples[i].timeStamp},${samples[i].auxData.readInt16BE()},${samples[i].boardTime}\n`);

        }
    },runTimeSec * 1000);
};
var timesSynced = 0;
var timesToSync = 2;
var timesToFailSync = 5;
var timeSyncActivated = false;
var samples = [];
// startRubyScreenFlasher();
var startHost = () => {
    ourBoard.connect(portNames.host).then(() => {
        ourBoard.on('ready',() => {
                ourBoard.streamStart()
                    .then(() => {
                        // Start the screen flasher program

                    })
                    .catch(err => {
                        endKindly();
                    })
            });
        ourBoard.on('sample',sample => {
            // If we are not sycned, then do that
            if (timeSyncActivated === false) {
                timeSyncActivated = true;
                ourBoard.syncClocksFull()
                    // .then(syncObj => {
                    //     if (syncObj.valid) {
                    //         console.log('first sync done');
                    //     }
                    //     return ourBoard.syncClocksFull();
                    // })
                    // .then(syncObj => {
                    //     if (syncObj.valid) {
                    //         console.log('2nd sync done');
                    //     }
                    //     return ourBoard.syncClocksFull();
                    // })
                    // .then(syncObj => {
                    //     if (syncObj.valid) {
                    //         console.log('3rd sync done');
                    //     }
                    //     return ourBoard.syncClocksFull();
                    // })
                    .then(syncObj => {
                        if (syncObj.valid) {
                            console.log('1st sync done');

                        }
                        startHardwareFlasher();
                        startLoggingSamples = true;
                    })
                    .catch(err => {
                        console.log(`sync err ${err}`);
                        endKindly();
                    });
            }
            if (startLoggingSamples && sample.hasOwnProperty("timeStamp") && sample.hasOwnProperty("boardTime")) {
                samples.push(sample);
            }
        });
    })
    .catch(err => {
        console.log(`connect ${err}`);
        endKindly();
    });
}

writeHeaderDataLog();
startHost();

process.on('exit', (code) => {
    if (child) {
        child.kill();
    }
    // Close the ruby screen flasher program
    if (ourBoard) {
        if (ourBoard.connected) {
            ourBoard.disconnect().then(() => {

            });
        }
    }
});
