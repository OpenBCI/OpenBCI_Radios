var OpenBCIBoard = require('openbci').OpenBCIBoard,
    ourBoard = new OpenBCIBoard({
        verbose:true
        // timeSync: true // Sync up with NTP servers in constructor
    }),
    fs = require('fs'),
    wstreamSample = fs.createWriteStream('timeSyncTest-samples5SyncLocal5.csv'),
    util = require('util'),
    exec = require('child_process').exec,
    robot = require("robotjs");

var child;

var portNames = {
    host: '/dev/cu.usbserial-DJ00DN7Z'
}

var writeHeaderDataLog = () => {
    wstreamSample.write(`Time Stamp,Sensor Value,Board Time\n`);
}

const sampleRate = 250;
var runTimeSec = 60; //seconds
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

var startRubyScreenFlasher = () => {
    child = exec('ruby ~/Documents/Arduino/libraries/OpenBCI_Radios/test/ruby/generate_MMN.rb',
        (error, stdout, stderr) => {
            console.log(`stdout: ${stdout}`);
            console.log(`stderr: ${stderr}`);
            if (error !== null) {
                console.log(`exec error: ${error}`);
            }
    });
};
var timesSynced = 0;
var timesToSync = 2;
var timesToFailSync = 5;
var timeSyncActivated = false;
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

        // ourBoard.on('synced',syncObj => {
        //     if (syncObj.valid) {
        //         timesSynced++;
        //         if (timesSynced >= timesToSync) {
        //             startRubyScreenFlasher();
        //             startLoggingSamples = true;
        //         } else {
        //             ourBoard.syncClocks()
        //                 .catch(err => {
        //                     console.log(err);
        //                     endKindly();
        //                 })
        //         }
        //     } else {
        //         console.log('bad sync');
        //         timesSynced++;
        //         if (timesSynced >= timesToFailSync) {
        //             endKindly();
        //         } else {
        //             ourBoard.syncClocks()
        //                 .catch(err => {
        //                     console.log(err);
        //                     endKindly();
        //                 });
        //         }
        //     }
        // })
        ourBoard.on('sample',sample => {
            // If we are not sycned, then do that
            if (timeSyncActivated === false) {
                timeSyncActivated = true;
                ourBoard.syncClocksFull()
                    .then(syncObj => {
                        if (syncObj.valid) {
                            console.log('first sync done');
                        }
                        return ourBoard.syncClocksFull();
                    })
                    .then(syncObj => {
                        if (syncObj.valid) {
                            console.log('2nd sync done');
                        }
                        return ourBoard.syncClocksFull();
                    })
                    .then(syncObj => {
                        if (syncObj.valid) {
                            console.log('3rd sync done');
                        }
                        return ourBoard.syncClocksFull();
                    })
                    .then(syncObj => {
                        if (syncObj.valid) {
                            console.log('4th sync done');

                        }
                        startRubyScreenFlasher();
                        startLoggingSamples = true;
                    })
                    .catch(err => {
                        endKindly();
                    });
            }
            if (startLoggingSamples && sample.hasOwnProperty("timeStamp") && sample.hasOwnProperty("boardTime")) {
                // console.log(`${sample.timeStamp},${sample.auxData.readInt16BE()},${sample.boardTime}`);
                wstreamSample.write(`${sample.timeStamp},${sample.auxData.readInt16BE()},${sample.boardTime}\n`);
                rawSampleCount++;
                if (rawSampleCount >= totalSamplesToGet) {
                    endKindly();
                }
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
    // Close the ruby screen flasher program
    robot.keyTap("escape");
    if (ourBoard) {
        if (ourBoard.connected) {
            ourBoard.disconnect().then(() => {

            });
        }
    }
});
