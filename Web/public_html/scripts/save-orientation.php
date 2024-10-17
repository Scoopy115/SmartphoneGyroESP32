<?php

header('Content-Type: application/json');

// Retrieve POST data
$postData = file_get_contents('php://input');
$data = json_decode($postData, true);

if (!$data) {
    http_response_code(400); // Bad Request
    echo json_encode(['error' => 'Invalid data format']);
    exit;
}

// Database connection
$servername = 'localhost';
$username = 'YourDBUsername';
$password = 'YourDBPassword';
$dbname = 'YourDBName';

$maxEntries = 500; // Set the maximum number of database entries

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    http_response_code(500); // Internal Server Error
    echo json_encode(['error' => 'Database connection failed']);
    exit;
}

// Delete old entries if set maximum limit is reached
$result = $conn->query('SELECT COUNT(*) as count FROM orientation_data');
if ($result && $result->num_rows > 0) {
    $row = $result->fetch_assoc();
    $totalCount = $row['count'];

    if ($totalCount >= $maxEntries) {
        $entriesToDelete = $totalCount - $maxEntries + 1; // Keep the latest $maxEntries entries
        $conn->query("DELETE FROM orientation_data ORDER BY timestamp ASC LIMIT $entriesToDelete");
    }
}

// SQL statement to insert data into database
$stmt = $conn->prepare('INSERT INTO orientation_data (alpha, beta, gamma) VALUES (?, ?, ?)');
$stmt->bind_param('ddd', $data['alpha'], $data['beta'], $data['gamma']);
$stmt->execute();

// Close connections
$stmt->close();
$conn->close();

// Respond with success
echo json_encode(['success' => true]);

?>
