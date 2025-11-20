const swaggerAutogen = require('swagger-autogen')();

const doc = {
  info: {
    title: 'Mi API',
    description: 'Descripción de la API',
  },
  host: 'localhost:3000',
  schemes: ['http'],
};

const outputFile = './swagger_output.json';
const endpointsFiles = ['./app.js']; 

/* Genera el archivo si no existe y lo actualiza si existe */
swaggerAutogen(outputFile, endpointsFiles, doc).then(() => {
  require('./app.js'); 
});
