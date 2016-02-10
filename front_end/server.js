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
var root_abs_path = __dirname; 
var public_dir_abs_path = root_abs_path + '/public';
var public_downloads_dir_abs_path = public_dir_abs_path + '/downloads'
var public_downloads_users_dir_abs_path = public_downloads_dir_abs_path + '/users'
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
var mkdirp = require('mkdirp');

var morgan       = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser   = require('body-parser');
var session      = require('express-session');

app.redis_connection = redis.createClient();
app.redis_connection.on("error", function (err) {
    console.error(err);
});

// app_r

var http = require('http');
var io = require('socket.io');
var server = http.createServer(app);
var listener = io.listen(server);

var rabbit_connection = amqp.createConnection({ host: config_settings.rabbitmq_host });
rabbit_connection.on('error', function (err) {
    console.error(err);
});

listener.on('connection', function (socket) {
    socket.on('subscribe_redis', function (data) {
        socket.join(data.channel);
    });

    socket.on('publish_redis', function (msg) {
        redis_pub.publish(config_settings.pub_sub_channel_in, msg);
    });
    
    socket.on('publish_rabbitmq', function (msg) {
        var in_json = JSON.parse(msg);
        var user_package_dir = public_downloads_users_dir_abs_path + '/' + in_json.email;
        mkdirp(user_package_dir, function(err) {
          if (err) {
            console.error(err);
            return;
          }
 
          var rpc = new (require('./app/amqprpc'))(rabbit_connection);
          var branding_variables = '-DUSER_SPECIFIC_DEFAULT_LOGIN=' + in_json.email + ' -DUSER_SPECIFIC_DEFAULT_PASSWORD=' + in_json.password
          + ' -DUSER_SPECIFIC_DEFAULT_DOMAIN=' + in_json.domain + ' -DUSER_SPECIFIC_CONTENT_PATH=' + in_json.content_path;
          
          var request_data_json = {
              'branding_variables': branding_variables,
              'platform': in_json.platform,
              'arch': in_json.arch,
              'package_type' : in_json.package_type,
              'destination' : user_package_dir
          };
          console.log("request_data_json", request_data_json);
          
          rpc.makeRequest(in_json.platform, in_json.email, request_data_json, function response(err, response) {
              if (err) {
                console.error(err);
              } else {
                console.log("response", response);
              }
          });
        });
    });
});

var redis_sub = redis.createClient();
var redis_pub = redis.createClient();

redis_sub.on("error", function (err) {
    console.error(err);
});

redis_pub.on("error", function (err) {
    console.error(err);
});

redis_sub.on('ready', function() {
    redis_sub.subscribe(config_settings.pub_sub_channel_out, config_settings.pub_sub_channel_client_state);
});

redis_sub.on("message_redis", function(channel, message){
    var resp = {'text': message, 'channel':channel}
    listener.in(channel).emit('message', resp);
});

// configuration ===============================================================
mongoose.connect(configDB.url); // connect to our database

require('./config/passport')(passport); // pass passport for configuration

// set up our express application
app.use(express.static(public_dir_abs_path));
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
