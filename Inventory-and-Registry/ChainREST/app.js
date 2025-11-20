var createError = require('http-errors');
var express = require('express');
var path = require('path');
var cookieParser = require('cookie-parser');
var logger = require('morgan');

var swaggerUi = require('swagger-ui-express');
var swaggerDocument = require('./swagger_output.json');
var indexRouter = require('./routes/index');
var chainRouter = require('./routes/chain')

var app = express();


app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'pug');

app.use(logger('dev'));
app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

app.use('/', indexRouter);
app.use('/chain', chainRouter);
app.use('/api-docs', swaggerUi.serve, swaggerUi.setup(swaggerDocument));


app.use(function(req, res, next) {
  next(createError(404));
});


app.use(function(err, req, res, next) {
  
  const errorResponse = {
    error: {
      status: err.status || 500,
      message: err.message,
    }
  };

  
  if (req.app.get('env') === 'development') {
    errorResponse.error.stack = err.stack;
  }

  
  res.status(err.status || 500).json(errorResponse);
});

module.exports = app;
