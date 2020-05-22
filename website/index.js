#!/usr/bin/env node
const express = require('express'),
      app = express(),
      port = 42069;

app.get('/', (req, res) => {
	console.log(req);
	res.sendFile("./index.html");
});

app.get('/start/:image', (req, res) => {
});

app.get('/stop/:sock', (req, res) => {
});

app.get('/clean/:sock', (req, res) => {
});

app.get('/check/:sock', (req, res) => {
});

app.listen(port, () => {
	console.log(`Listening on ${port}...`);
});
