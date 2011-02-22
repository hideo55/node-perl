node-perl
=========

Embed Perl interpreter for node.js

## Install

    #>git clone git://github.com/hideo55/node-perl.git
    #>cd node-perl
    #>node-waf configure
    #>node-waf build
    #>node-waf install

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