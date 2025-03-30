#!/usr/bin/php-cgi
<?php
// Set the content type header for HTTP response
header("Content-Type: text/html");

// Array of fortune cookie messages
$fortunes = [
    "You will find great success in your near future.",
    "A pleasant surprise is waiting for you.",
    "Your talents will be recognized and rewarded.",
    "Good things come to those who wait patiently.",
    "A new adventure awaits you around the corner.",
    "Wisdom will guide you to make the right choices.",
    "Happiness is not a destination, it's a journey.",
    "Your creativity will lead you to new opportunities.",
    "A friend will bring you unexpected joy soon.",
    "The best is yet to come - stay optimistic."
];

// Get a random fortune
$randomFortune = $fortunes[array_rand($fortunes)];

// HTML output matching your site's style
echo "<!DOCTYPE html>\n";
echo "<html lang=\"en\">\n";
echo "<head>\n";
echo "<meta charset=\"UTF-8\">\n";
echo "<title>Webserv - Your Fortune</title>\n";
echo "<style>\n";
echo "body {";
echo "    margin: 0;";
echo "    font-family: 'Courier New', monospace;";
echo "    background: linear-gradient(45deg, #cc0000, #ff3333, #990000, #ff6666);";
echo "    background-size: 400% 400%;";
echo "    animation: fadeBackground 10s ease-in-out infinite;";
echo "    color: #ffcc00;";
echo "    perspective: 1000px;";
echo "}";
echo ".container {";
echo "    display: flex;";
echo "    flex-direction: column;";
echo "    align-items: center;";
echo "    padding: 40px;";
echo "}";
echo "h1 {";
echo "    font-size: 48px;";
echo "    text-transform: uppercase;";
echo "    letter-spacing: 4px;";
echo "    text-shadow: 0 0 20px #ff3333;";
echo "}";
echo "p {";
echo "    font-size: 24px;";
echo "    text-shadow: 0 0 10px #ff3333;";
echo "    margin: 20px;";
echo "}";
echo "a {";
echo "    color: #ffcc00;";
echo "    background: transparent;";
echo "    border: 2px solid #ffcc00;";
echo "    padding: 10px 20px;";
echo "    border-radius: 10px;";
echo "    text-decoration: none;";
echo "    font-weight: bold;";
echo "    transition: all 0.3s;";
echo "}";
echo "a:hover {";
echo "    background: #ffcc00;";
echo "    color: #cc0000;";
echo "    transform: scale(1.1);";
echo "    box-shadow: 0 0 20px #ffcc00;";
echo "}";
echo "@keyframes fadeBackground {";
echo "    0% { background-position: 0% 0%; }";
echo "    50% { background-position: 100% 100%; }";
echo "    100% { background-position: 0% 0%; }";
echo "}";
echo "</style>\n";
echo "</head>\n";
echo "<body>\n";
echo "<div class=\"container\">\n";
echo "<h1>Your Fortune</h1>\n";
echo "<p>" . htmlspecialchars($randomFortune) . "</p>\n";
echo "<a href=\"/\">Back to Main Page</a>\n";
echo "<a href=\"/cgi-bin/fortune.php\">Another Fortune</a>\n";
echo "</div>\n";
echo "</body>\n";
echo "</html>\n";
?>
