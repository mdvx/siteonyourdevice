// server.js

const CHANNEL_IN = 'COMMANDS_IN';
const CHANNEL_OUT = 'COMMANDS_OUT';
const CHANNEL_CLIENTS_STATE = 'CLIENTS_STATE';
const NODE_PORT = 3000;

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

// set up ======================================================================
// get all the tools we need
var express  = require('express');
var app      = express();
var port     = process.env.PORT || 8080;
var mongoose = require('mongoose');
var redis = require('redis');
var passport = require('passport');
var flash    = require('connect-flash');

var morgan       = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser   = require('body-parser');
var session      = require('express-session');

var redis_client = redis.createClient();
redis_client.on("error", function (err) {
    console.log("redis_client error" + err);
});

redis_client.on("error", function (err) {
    console.log("redis_client error " + err);
});

// app_r

var http = require('http');
var io = require('socket.io');
var server = http.createServer(app);
var listener = io.listen(server);

listener.on('connection', function (socket) {
  console.log('Connection to client established');
  socket.on('newsresponse', function (data) {
    console.log(data);
  });
 
  socket.on('disconnect',function(){
    console.log('Server has disconnected');
  }); 
});

server.listen(NODE_PORT);

var redis_sub = redis.createClient();
var redis_pub = redis.createClient();

redis_sub.on("error", function (err) {
    console.log("redis_sub error" + err);
});

redis_pub.on("error", function (err) {
    console.log("redis_pub error " + err);
});

io.sockets.on('connection', function (socket) {
    socket.on('subscribe', function (data) {
        socket.join(data.channel);
    });

    socket.on('publish', function (msg) {
        redis_pub.publish(CHANNEL_IN, msg);
    });
});

redis_sub.on('ready', function() {
    redis_sub.subscribe(CHANNEL_OUT, CHANNEL_CLIENTS_STATE);
});

redis_sub.on("message", function(channel, message){
    var resp = {'text': message, 'channel':channel}
    io.sockets.in(channel).emit('message', resp);
});

var configDB = require('./config/database.js');

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
require('./app/routes.js')(app, passport, redis_client); // load our routes and pass in our app and fully configured passport

// launch ======================================================================
app.listen(port);