-- prepared for postgresql

CREATE TABLE IF NOT EXISTS sandboxes (
	pid INTEGER PRIMARY KEY,
	config VARCHAR(255) UNIQUE NOT NULL,
	console SMALLINT UNIQUE NOT NULL, -- may be changed to varchar for unix socket
	qmpsock SMALLINT UNIQUE NOT NULL,
	creation TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);
