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

function parseCookies (request) {
    var list = {},
        rc = request.headers.cookie;

    rc && rc.split(';').forEach(function( cookie ) {
        var parts = cookie.split('=');
        list[parts.shift().trim()] = decodeURI(parts.join('='));
    });

    return list;
}

var app_r = require('http').createServer(handler);
var io = require('socket.io').listen(app_r);

function handler (req, res) {
    fs.readFile('server_details.ejs',
    function (err, data) {
        if (err) {
            res.writeHead(500);
            return res.end('Error loading server_details.html, folder' + __dirname);
        }
        
        var user_id = get_url_parameter_url(req.url, 'id');
        var user_cookie_hash = get_url_parameter_url(req.url, 'hash');
        
	var cookies = parseCookies(req); 
        var req_user_id = cookies['id'];
        var req_user_cookie_hash = cookies['hash'];
        if(req_user_id === user_id && req_user_cookie_hash === user_cookie_hash){
	    res.writeHead(200);
        res.end(data);
	}
	else{
	    res.writeHead(403);
            return res.end('Access Denied');

	}        
    });
}

app_r.listen(NODE_PORT);

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