var fs = require('fs');
var openBCIBoard = require('openbci');
var k = openBCIBoard.OpenBCIConstants;
var ourBoard = new openBCIBoard.OpenBCIBoard({verbose:true});
var wstream = fs.createWriteStream('enduranceTest.txt');

var portNames = {
    host: '/dev/tty.usbserial-DB00JAKZ'
}

var badPackets = 0;
var goodPackets = 0;
var rawSampleCount = 0;
var sampleRecievedCounter = 0;

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
            });
        ourBoard.on('sample',function(sample) {
            if (rawSampleCount % 1500 === 0) {
                wstream.write(`\n\nSummary:\n\tTotal Packets: ${rawSampleCount}\n\t\tGood Packets: ${goodPackets}\n\t\tBad Packets: ${badPackets}\n\t`);
                writeOutDateAndTime();
                wstream.write(`\n\n`);
                // console.log(`Summary:\n\tTotal Packets: ${rawSampleCount}\n\t\tGood Packets: ${goodPackets}\n\t\tBad Packets: ${badPackets}\n`);
            }
            rawSampleCount++;
            if (sample.sampleNumber === sampleRecievedCounter) {
                goodPackets++;
                sampleRecievedCounter++;
            } else {
                console.log(`err: expected ${sampleRecievedCounter} got ${sample.sampleNumber}`);
                wstream.write(`err: expected ${sampleRecievedCounter} got ${sample.sampleNumber} at `);
                writeOutDate();
                wstream.write(`\n`);
                badPackets++;
                sampleRecievedCounter = sample.sampleNumber + 1;
            }

            // Loop the counter back to 0
            if (sampleRecievedCounter > 255) sampleRecievedCounter = 0;

        });
    })
}

wstream.write(`OpenBCI Endurance Test\nPush The World LLC\n`);
wstream.write(`START!\n`);
writeOutDateAndTime();
startHost();


process.on('exit', () => {
    if (ourBoard) {
        if (ourBoard.connected) {
            ourBoard.disconnect().then(() => {
            });
        }
    }
});
