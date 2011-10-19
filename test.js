var Perl = require('./perl').Perl;
var perl = new Perl();
var assert = require('assert');

perl.Run({
    opts : ["-Mfeature=say","-e","say 'Hello world';"]
}, function(e,out,err){
  assert.equal(e,null, 'Error is not occured.');
  assert.equal(out, "Hello world\n");
});