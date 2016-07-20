// http://nodejs.org/api.html#_child_processes
const OpenBCIBoard = require('openbci').OpenBCIBoard,
        sys = require('sys'),
        exec = require('child_process').exec;

var child;
var ourBoard = new OpenBCIBoard({
    verbose:true,
    timeSync:true
});

console.log(__dir__);

// executes `pwd`
// child = exec("ruby ~/Documents", function (error, stdout, stderr) {
//   sys.print('stdout: ' + stdout);
//   sys.print('stderr: ' + stderr);
//   if (error !== null) {
//     console.log('exec error: ' + error);
//   }
// });
