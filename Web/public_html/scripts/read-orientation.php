<?php

// Database connection
$servername = 'localhost';
$username = 'YourDBUsername';
$password = 'YourDBPassword';
$dbname = 'YourDBName';

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    http_response_code(500); // Internal Server Error
    echo json_encode(['error' => 'Database connection failed']);
    exit;
}

// Fetch the orientation data with the highest ID (latest data) from the database
$query = "SELECT alpha, beta, gamma FROM orientation_data WHERE id = (SELECT MAX(id) FROM orientation_data)";
$result = $conn->query($query);

// Check if the query was successful
if (!$result) {
    http_response_code(500); // Internal Server Error
    echo json_encode(['error' => 'Failed to execute query']);
    exit;
}

// Fetch the data from the result
$data = $result->fetch_assoc();

// Close the database connection
$conn->close();

// Return the data as JSON
header('Content-Type: application/json');
echo json_encode($data);
?>
