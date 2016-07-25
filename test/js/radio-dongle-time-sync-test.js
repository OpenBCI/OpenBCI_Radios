var serialPort = require('serialport'),
            fs = require('fs'),
   wstreamStim = fs.createWriteStream('Hardware_timestamp-stim1.csv');

var portNames = {
    slave: '/dev/cu.usbserial-DJ00DN7Z'
}

var interval = 0.1 //
var intervalMs = interval * 1000;
var ti = Math.floor((1.0/(interval * 2))*1*60)
var timestamp = [];
var trial = 0;
var wrapedUp = false;
console.log(ti);

var sleep = miliseconds => {
   return new Promise(function(resolve, reject) {
       var currentTime = Date.now();
       while (currentTime + miliseconds >= Date.now()) {}
       resolve();
   });
}
var writeHeaderDataLog = () => {
    wstreamStim.write(`timestamp,target\n`);
}

var deviceSerial;


process.on('message', m => {
    console.log('CHILD GOT MESSAGE:', m);
    if (m.body === "stop") {
        wrapup();
    } else if (m.body === "start") {
        start();
    }
})


var writeAndDrain = data => {
    return new Promise((resolve,reject) => {
        if(!deviceSerial) reject('Serial port not open');
        deviceSerial.write(data,(error,results) => {
            if(results) {
                deviceSerial.drain(function() {
                    resolve();
                });
            } else {
                console.log('Error [writeAndDrain]: ' + error);
                reject(error);
            }
        })
    });
};

var loop = () => {
    timestamp.push(Date.now());
    writeAndDrain(new Buffer("1"))
        .then(() => {
            return sleep(intervalMs);
        })
        .then(() => {
            return writeAndDrain("0");
        })
        .then(() => {
            return sleep(intervalMs);
        })
        .then(() => {
            trial++;
            loop();
            // if (trial < ti) {
            //     loop();
            // } else {
            //     wrapup();
            // }
        })
        .catch(err => {
            console.log(err);
            proccess.exit(1);
        });
}

var start = () => {
    deviceSerial = new serialPort.SerialPort(portNames.slave, {
        baudRate: 115200
    },(err) => {
        if (err) {
            console.log(`error opening serialport: ${err}`);
            process.exit(1);
        }
    });
    deviceSerial.on('open', () => {
        console.log('serialport open');
        loop();
    })

    deviceSerial.on('close', () => {
        console.log('closed');
        // process.exit(0);
    });

    deviceSerial.on('error', err => {
        console.log(`error ${err}`);
        // process.exit(1);
    });
}

var wrapup = () => {
    // Write the header
    writeHeaderDataLog();

    var len = timestamp.length;
    for(var i = 0; i < len; i++) {
        console.log(`${timestamp[i]},1`);
        wstreamStim.write(`${timestamp[i]},1\n`)
    }
    console.log(`done`);
    deviceSerial.close();
    process.send({body:"childDone"});
}

process.on('exit', (code) => {
    // Close the ruby screen flasher program
    // wrapup();
});
