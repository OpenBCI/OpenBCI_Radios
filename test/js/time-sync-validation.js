var OpenBCIBoard = require('openbci').OpenBCIBoard,
    ourBoard = new OpenBCIBoard({
        verbose:true,
        timeSync: true // Sync up with NTP servers in constructor
    }),
    fs = require('fs'),
    wstreamSample = fs.createWriteStream('timeSyncTest-samples.txt'),
    wstreamSent = fs.createWriteStream('timeSyncTest-sentTimes.txt');

var portNames = {
    host: '/dev/cu.usbserial-DB00JAKZ'
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

var writeOutDate = () => {
    wstream.write(`${new Date().toISOString().replace(/T/, ' ').replace(/\..+/, '')}`);
}
var writeOutDateAndTime = () => {
    wstream.write(`Date and time: `);
    writeOutDate();
}

var startHost = () => {
    ourBoard.connect(portNames.host).then(() => {
        ourBoard.on('ready',function() {
                ourBoard.streamStart();
                // ourBoard.syncClocksStart().catch(err => console.log('sync err',err));
            });
        // ourBoard.on('rawDataPacket', rawDataPacket => {
        //     console.log('rawDataPacket',rawDataPacket);
        // })
        var timeSyncActivated = false;
        ourBoard.on('sample',function(sample) {
            // If we are not sycned, then do that
            if (timeSyncActivated === false) {
                timeSyncActivated = true;
                ourBoard.syncClocks().then(() => {
                    console.log('starting variable');
                    setInterval(() => {
                        if (isCycleHigh) {
                            // write low
                            ourBoard._writeAndDrain(new Buffer([0xF0,0x08])).then(() => {
                                var timeSent = ourBoard.sntpNow();
                                wstreamSent.write(`low,${timeSent}\n`);
                                timeSentLowArray.push(timeSent);
                            });
                            isCycleHigh = false;
                        } else {
                            // write high
                            ourBoard._writeAndDrain(new Buffer([0xF0,0x09])).then(() => {
                                var timeSent = ourBoard.sntpNow();
                                wstreamSent.write(`high,${timeSent}\n`);
                                timeSentHighArray.push(timeSent);
                            });
                            isCycleHigh = true;
                        }
                    }, 500);
                }).catch(err => {
                    process.exit(err);
                });
            }
            rawSampleCount++;
            // console.log(`got sample ${sample.sampleNumber} expecting ${sampleRecievedCounter}`);
            if (sample.timeStamp) {
                // console.log(sample.timeStamp);
                wstreamSample.write(`${sample.auxData.readInt16BE()},${sample.timeStamp}\n`);
            }

            if (rawSampleCount >= totalSamplesToGet) {
                process.exit();
            }

        });
    })
}

startHost();

process.on('exit', (code) => {
    if (ourBoard) {
        if (ourBoard.connected) {
            ourBoard.disconnect().then(() => {

            });
        }
    }
});
