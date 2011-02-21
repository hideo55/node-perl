node-perl
=========

Embed Perl interpreter for node.js

## Tutorial

var Perl = require('perl').Perl();
var perl = new Perl();

perl.Run({
	opts : ["-Mfeature=say","-e","say 'Hello world'"]
}, function(out,err){
	console.log(out);
});

perl.Run({
	script : 'example.pl',
	args : ['foo', 'bar']
});

## API

### Run(options,[callback])

## License

node-perl is licensed under the MIT license.