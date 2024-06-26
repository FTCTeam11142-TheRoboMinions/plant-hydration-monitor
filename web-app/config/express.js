const express = require('express'),
  path = require('path'),
  glob = require('glob'),
  favicon = require('serve-favicon'),
  logger = require('morgan'),
  cookieParser = require('cookie-parser'),
  bodyParser = require('body-parser'),
  compress = require('compression'),
  methodOverride = require('method-override');

module.exports = function(app, config) {
  const env = process.env.NODE_ENV || 'development';
  app.locals.ENV = env;
  app.locals.ENV_DEVELOPMENT = env == 'development';

  // app.use(favicon(path.join(config.root, 'public', 'img', 'favicon.ico')));
  app.use(logger('dev'));
  app.use(bodyParser.json());
  app.use(bodyParser.urlencoded({
    extended: true
  }));
  app.use(cookieParser());
  app.use(compress());
  app.use(express.static(path.join(config.root, 'public')));
  app.use(methodOverride());

  let controllers = glob.sync(path.join(config.root, 'app', 'controllers', '**', '*.js').replace(/\\/g, '/'));
  controllers.forEach(function (controller) {
    console.log(`Loading controller: ${controller}`);
    require(controller)(app);
  });

  app.use(function (req, res, next) {
    let err = new Error('Not Found');
    err.status = 404;
    next(err);
  });
  
  // Cannot setup "render" until a view engine is setup
  // Suggestion: Nunjucks, https://mozilla.github.io/nunjucks/
  /*if(app.get('env') === 'development'){
    app.use(function (err, req, res, next) {
      res.status(err.status || 500);
      res.render('error', {
        message: err.message,
        error: err,
        title: 'error'
      });
    });
  }

  app.use(function (err, req, res, next) {
    res.status(err.status || 500);
      res.render('error', {
        message: err.message,
        error: {},
        title: 'error'
      });
  });*/
};
