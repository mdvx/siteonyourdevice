// server.js

//get url field
function get_url_parameter_url(url, sParam) 
{
    var sPageURL = decodeURIComponent(url),
        sURLVariables = sPageURL.split('&'),
        sParameterName,
        i;

    for (i = 0; i < sURLVariables.length; i++) {
        sParameterName = sURLVariables[i].split('=');

        if (sParameterName[0] === sParam) {
            return sParameterName[1] === undefined ? true : sParameterName[1];
        }
    }
}

// load configs
var configDB = require('./config/database.js');
var config_settings = require('./config/settings.js');

// set up ======================================================================
// get all the tools we need
var express  = require('express');
var app      = express();
var port     = process.env.PORT || config_settings.http_server_port;
var mongoose = require('mongoose');
var redis = require('redis');
var passport = require('passport');
var flash    = require('connect-flash');
var amqp = require('amqp');

var morgan       = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser   = require('body-parser');
var session      = require('express-session');

app.redis_connection = redis.createClient();
app.redis_connection.on("error", function (err) {
    console.log("redis_client error: " + err);
});

// app_r

var http = require('http');
var io = require('socket.io');
var server = http.createServer(app);
var listener = io.listen(server);

listener.on('connection', function (socket) {
    socket.on('subscribe', function (data) {
        socket.join(data.channel);
    });

    socket.on('publish_redis', function (msg) {
        redis_pub.publish(config_settings.pub_sub_channel_in, msg);
    });
    
    socket.on('publish_rabbitmq', function (msg) {        
        var exchange = rabbit_connection.exchange('');
        var in_json = JSON.parse(msg);
        var branding_variables = '-DUSER_SPECIFIC_DEFAULT_LOGIN=' + in_json.email + ' -DUSER_SPECIFIC_DEFAULT_PASSWORD' + in_json.password
        + ' -DUSER_SPECIFIC_DEFAULT_DOMAIN' + in_json.domain + ' -DUSER_SPECIFIC_CONTENT_PATH' + in_json.content_path;
        
        var request_data_json = {
            'branding_variables': branding_variables,
            'platform': in_json.platform,
            'arch': in_json.arch,
            'package_type' : in_json.package_type
        }
        console.log("request_data_json : " + request_data_json);
        exchange.publish(in_json.platform, request_data_json)
    });
});

var redis_sub = redis.createClient();
var redis_pub = redis.createClient();

redis_sub.on("error", function (err) {
    console.log("redis_sub error: " + err);
});

redis_pub.on("error", function (err) {
    console.log("redis_pub error: " + err);
});

redis_sub.on('ready', function() {
    redis_sub.subscribe(config_settings.pub_sub_channel_out, config_settings.pub_sub_channel_client_state);
});

redis_sub.on("message_redis", function(channel, message){
    var resp = {'text': message, 'channel':channel}
    listener.in(channel).emit('message', resp);
});

rabbit_connection = amqp.createConnection({ host: config_settings.rabbitmq_host });
rabbit_connection.on('error', function (err) {
    console.log("rabbit_connection error: " + err);
});

// configuration ===============================================================
mongoose.connect(configDB.url); // connect to our database

require('./config/passport')(passport); // pass passport for configuration

// set up our express application
app.use(express.static('public'));
app.use(morgan('dev')); // log every request to the console
app.use(cookieParser()); // read cookies (needed for auth)
app.use(bodyParser.json()); // get information from html forms
app.use(bodyParser.urlencoded({ extended: true }));

app.set('view engine', 'ejs'); // set up ejs for templating

// required for passport
app.use(session({ secret: 'siteonyourdevice' })); // session secret
app.use(passport.initialize());
app.use(passport.session()); // persistent login sessions
app.use(flash()); // use connect-flash for flash messages stored in session

// routes ======================================================================
require('./app/routes.js')(app, passport, config_settings); // load our routes and pass in our app and fully configured passport

// launch ======================================================================
app.listen(port);
console.log('Http server ready for requests');
server.listen(config_settings.redis_pub_sub_port);
