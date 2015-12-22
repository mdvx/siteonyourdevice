// load the things we need
var mongoose = require('mongoose');

// define the schema for our user model
var templatesSchema = mongoose.Schema({
    name : String,
    download_link : String,
    http_handlers_utls : [{
                            key : String,
                            value : String
                            }]
});

// create the model for users and expose it to our app
module.exports = mongoose.model('Templates', templatesSchema);
