var binding;

try {
  binding = require(__dirname + '/build/Release/perl');
} catch(e) {
  binding = require(__dirname + '/build/default/perl');
}

exports.Perl = binding.Perl;
exports.version = binding.version;
