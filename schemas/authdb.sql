-- Create the Authentication Database
CREATE DATABASE IF NOT EXISTS authentication_db;
USE authentication_db;

-- Create the user table
CREATE TABLE user (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  last_login TIMESTAMP,
  creation_date DATETIME,
  INDEX(last_login)
);

-- Create the web_auth table
CREATE TABLE web_auth (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  email VARCHAR(255) NOT NULL,
  salt CHAR(64) NOT NULL,
  hashed_password CHAR(64) NOT NULL,
  userId BIGINT NOT NULL,
  INDEX(email)
);

CREATE USER 'admin'@'localhost' IDENTIFIED BY 'admin123';
GRANT ALL PRIVILEGES ON authentication_db.* TO 'admin'@'localhost' WITH GRANT OPTION;
FLUSH PRIVILEGES;
