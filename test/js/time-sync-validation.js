var OpenBCIBoard = require('openbci').OpenBCIBoard,
    ourBoard = new OpenBCIBoard({
        verbose:true,
        timeSync: true // Sync up with NTP servers in constructor
    }),
    fs = require('fs'),
    wstreamSample = fs.createWriteStream('timeSyncTest-samples.csv'),
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
var timeSyncActivated = false;
// startRubyScreenFlasher();
var startHost = () => {
    ourBoard.connect(portNames.host).then(() => {
        ourBoard.on('ready',() => {
                ourBoard.streamStart()
                    .then(() => {
                        // Start the screen flasher program
                        startRubyScreenFlasher();
                    })
                    .catch(err => {
                        endKindly();
                    })
            });


        ourBoard.on('sample',sample => {
            // If we are not sycned, then do that
            if (timeSyncActivated === false) {
                timeSyncActivated = true;
                ourBoard.syncClocks().then(() => {
                    // Jump for joy?

                }).catch(err => {
                    endKindly();
                });
            }
            if (sample.hasOwnProperty("timeStamp") && sample.hasOwnProperty("boardTime")) {
                console.log(`${sample.timeStamp},${sample.auxData.readInt16BE()},${sample.boardTime}`);
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
