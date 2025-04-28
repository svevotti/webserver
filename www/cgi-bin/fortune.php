#!/usr/bin/php-cgi
<?php
// Seed the random number generator with a unique value per request
srand((int) (microtime(true) * 1000000));

// Error page function
function send_error($code, $message) {
    header("Status: $code $message");
    header("Content-Type: text/html");
    echo "<!DOCTYPE html>\n";
    echo "<html lang=\"en\">\n";
    echo "<head>\n";
    echo "<meta charset=\"UTF-8\">\n";
    echo "<title>Webserv - $code $message</title>\n";
    echo "<style>\n";
    echo "body {\n";
    echo "    margin: 0;\n";
    echo "    font-family: 'Courier New', monospace;\n";
    echo "    background: linear-gradient(135deg, #cc0000, #ff3333, #990000, #ff6666, #cc0000);\n";
    echo "    background-size: 500% 500%;\n";
    echo "    animation: jazzBackground 8s ease-in-out infinite;\n";
    echo "    color: #ffcc00;\n";
    echo "    perspective: 1200px;\n";
    echo "    overflow-x: hidden;\n";
    echo "    position: relative;\n";
    echo "    min-height: 100vh;\n";
    echo "}\n";
    echo ".container {\n";
    echo "    display: flex;\n";
    echo "    flex-direction: column;\n";
    echo "    align-items: center;\n";
    echo "    padding: 20px;\n";
    echo "    position: relative;\n";
    echo "    z-index: 1;\n";
    echo "    transform-style: preserve-3d;\n";
    echo "}\n";
    echo "h1 {\n";
    echo "    font-size: 72px;\n";
    echo "    text-transform: uppercase;\n";
    echo "    letter-spacing: 10px;\n";
    echo "    text-align: center;\n";
    echo "    animation: glowJazz 1.5s infinite alternate;\n";
    echo "    text-shadow: 0 0 25px #ff3333, 0 0 50px #ff6666, 0 0 75px #cc0000;\n";
    echo "    margin-top: 50px;\n";
    echo "    margin-bottom: 20px;\n";
    echo "    transform: translateZ(50px) rotateX(10deg);\n";
    echo "    position: relative;\n";
    echo "    z-index: 2;\n";
    echo "}\n";
    echo ".error-zone {\n";
    echo "    background: rgba(255, 51, 0, 0.85);\n";
    echo "    padding: 25px;\n";
    echo "    border: 4px solid #ffcc00;\n";
    echo "    border-radius: 25px;\n";
    echo "    width: 85%;\n";
    echo "    max-width: 700px;\n";
    echo "    margin-bottom: 30px;\n";
    echo "    transform: rotateY(-15deg) rotateX(-5deg);\n";
    echo "    transition: transform 0.6s, box-shadow 0.6s;\n";
    echo "    box-shadow: 0 0 20px rgba(255, 204, 0, 0.7);\n";
    echo "}\n";
    echo ".error-zone:hover {\n";
    echo "    transform: rotateY(0deg) rotateX(0deg) translateZ(80px);\n";
    echo "    box-shadow: 0 0 40px #ff3333, 0 0 60px #ff6666;\n";
    echo "}\n";
    echo "h2 {\n";
    echo "    font-size: 28px;\n";
    echo "    margin-bottom: 20px;\n";
    echo "    text-shadow: 0 0 15px #ff3333, 0 0 30px #ff6666;\n";
    echo "    animation: pulseText 2s infinite;\n";
    echo "}\n";
    echo "p {\n";
    echo "    font-size: 18px;\n";
    echo "    line-height: 1.6;\n";
    echo "    text-shadow: 0 0 10px #ff3333;\n";
    echo "    animation: textPulse 2s infinite ease-in-out;\n";
    echo "}\n";
    echo "a {\n";
    echo "    color: #ffcc00;\n";
    echo "    background: rgba(0, 0, 0, 0.5);\n";
    echo "    border: 3px solid #ffcc00;\n";
    echo "    padding: 12px 25px;\n";
    echo "    border-radius: 15px;\n";
    echo "    text-decoration: none;\n";
    echo "    font-weight: bold;\n";
    echo "    cursor: pointer;\n";
    echo "    transition: all 0.4s;\n";
    echo "    display: inline-block;\n";
    echo "    text-align: center;\n";
    echo "    margin: 10px;\n";
    echo "    font-size: 18px;\n";
    echo "    transform: translateZ(20px);\n";
    echo "}\n";
    echo "a:hover {\n";
    echo "    background: #ff3333;\n";
    echo "    color: #ffcc00;\n";
    echo "    transform: scale(1.15) rotateX(15deg) translateZ(40px);\n";
    echo "    box-shadow: 0 0 25px #ffcc00, 0 0 50px #ff6666;\n";
    echo "}\n";
    echo ".bg-anim {\n";
    echo "    position: absolute;\n";
    echo "    background: rgba(255, 51, 0, 0.7);\n";
    echo "    border-radius: 50%;\n";
    echo "    animation: floatJazz 5s infinite ease-in-out;\n";
    echo "    z-index: 0;\n";
    echo "    box-shadow: 0 0 20px #ffcc00;\n";
    echo "}\n";
    echo ".bg-anim:nth-child(1) { width: 120px; height: 120px; top: 15%; left: 10%; animation-delay: 0s; }\n";
    echo ".bg-anim:nth-child(2) { width: 90px; height: 90px; top: 30%; right: 15%; animation-delay: 1s; }\n";
    echo ".bg-anim:nth-child(3) { width: 140px; height: 140px; bottom: 25%; left: 20%; animation-delay: 2s; }\n";
    echo ".bg-anim:nth-child(4) { width: 80px; height: 80px; bottom: 10%; right: 20%; animation-delay: 3s; }\n";
    echo ".bg-anim:nth-child(5) { width: 110px; height: 110px; top: 40%; left: 45%; animation-delay: 4s; }\n";
    echo "@keyframes jazzBackground {\n";
    echo "    0% { background-position: 0% 0%; }\n";
    echo "    25% { background-position: 50% 50%; }\n";
    echo "    50% { background-position: 100% 100%; }\n";
    echo "    75% { background-position: 50% 50%; }\n";
    echo "    100% { background-position: 0% 0%; }\n";
    echo "}\n";
    echo "@keyframes glowJazz {\n";
    echo "    0% { transform: translateZ(50px) scale(1); text-shadow: 0 0 25px #ff3333, 0 0 50px #ff6666; }\n";
    echo "    100% { transform: translateZ(70px) scale(1.1); text-shadow: 0 0 40px #ff3333, 0 0 80px #ff6666; }\n";
    echo "}\n";
    echo "@keyframes pulseText {\n";
    echo "    0% { transform: scale(1); }\n";
    echo "    50% { transform: scale(1.05); }\n";
    echo "    100% { transform: scale(1); }\n";
    echo "}\n";
    echo "@keyframes floatJazz {\n";
    echo "    0% { transform: translateZ(0) rotate(0deg); }\n";
    echo "    50% { transform: translateZ(120px) rotate(180deg); }\n";
    echo "    100% { transform: translateZ(0) rotate(360deg); }\n";
    echo "}\n";
    echo "@keyframes textPulse {\n";
    echo "    0% { transform: scale(1); }\n";
    echo "    50% { transform: scale(1.02); }\n";
    echo "    100% { transform: scale(1); }\n";
    echo "}\n";
    echo "</style>\n";
    echo "</head>\n";
    echo "<body>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<h1>$code - $message</h1>\n";
    echo "<div class=\"container\">\n";
    echo "<div class=\"error-zone\">\n";
    echo "<h2>Funky Error!</h2>\n";
    #echo "<p>$message</p>\n"; // to be used in the absence of CGI taylored messages
    if ($code == 503) { // new
        echo "<p>Whoa, the Webserv band took a quick break to tune their cosmic guitars! We're just chilling backstage, but we'll be back with a funky beat in no time. Hang tight and keep the groove alive!</p>\n"; // new
    } elseif ($code == 504) { // new
        echo "<p>Oh snap, our server got lost in a galactic jam session out in the neon void! The signal’s taking a detour through the cosmos, but we’ll reconnect the vibes soon. Stay cool and retry that rhythm!</p>\n"; // new
    } else { // new
        echo "<p>$message</p>\n"; // new
    } // new
    echo "<a href=\"/\">BACK TO MAIN</a>\n";
    echo "</div>\n";
    echo "</div>\n";
    echo "</body>\n";
    echo "</html>\n";
    exit(0);
}

// Check HTTP method (redirect to static error page)
$method = getenv("REQUEST_METHOD");
if ($method !== "GET") {
    header("Status: 405 Method Not Allowed"); // new
    header("Location: /405.html"); // new
    exit(0); // new
}

// // Check HTTP method (ALTERNATIVE WAY: Make page on the go)
// $method = getenv("REQUEST_METHOD");
// if ($method !== "GET") {
//     header("Status: 405 Method Not Allowed");
//     echo "<!DOCTYPE html>\n";
//     echo "<html lang=\"en\">\n";
//     echo "<head>\n";
//     echo "<meta charset=\"UTF-8\">\n";
//     echo "<title>Webserv - 405 Method Not Allowed</title>\n";
//     echo "<style>\n";
//     echo "body {\n";
//     echo "    margin: 0;\n";
//     echo "    font-family: 'Courier New', monospace;\n";
//     echo "    background: linear-gradient(135deg, #cc0000, #ff3333, #990000, #ff6666, #cc0000);\n";
//     echo "    background-size: 500% 500%;\n";
//     echo "    animation: jazzBackground 8s ease-in-out infinite;\n";
//     echo "    color: #ffcc00;\n";
//     echo "    perspective: 1200px;\n";
//     echo "    overflow-x: hidden;\n";
//     echo "    position: relative;\n";
//     echo "    min-height: 100vh;\n";
//     echo "}\n";
//     echo ".container {\n";
//     echo "    display: flex;\n";
//     echo "    flex-direction: column;\n";
//     echo "    align-items: center;\n";
//     echo "    padding: 20px;\n";
//     echo "    position: relative;\n";
//     echo "    z-index: 1;\n";
//     echo "    transform-style: preserve-3d;\n";
//     echo "}\n";
//     echo "h1 {\n";
//     echo "    font-size: 72px;\n";
//     echo "    text-transform: uppercase;\n";
//     echo "    letter-spacing: 10px;\n";
//     echo "    text-align: center;\n";
//     echo "    animation: glowJazz 1.5s infinite alternate;\n";
//     echo "    text-shadow: 0 0 25px #ff3333, 0 0 50px #ff6666, 0 0 75px #cc0000;\n";
//     echo "    margin-top: 50px;\n";
//     echo "    margin-bottom: 20px;\n";
//     echo "    transform: translateZ(50px) rotateX(10deg);\n";
//     echo "    position: relative;\n";
//     echo "    z-index: 2;\n";
//     echo "}\n";
//     echo ".error-zone {\n";
//     echo "    background: rgba(255, 51, 0, 0.85);\n";
//     echo "    padding: 25px;\n";
//     echo "    border: 4px solid #ffcc00;\n";
//     echo "    border-radius: 25px;\n";
//     echo "    width: 85%;\n";
//     echo "    max-width: 700px;\n";
//     echo "    margin-bottom: 30px;\n";
//     echo "    transform: rotateY(-15deg) rotateX(-5deg);\n";
//     echo "    transition: transform 0.6s, box-shadow 0.6s;\n";
//     echo "    box-shadow: 0 0 20px rgba(255, 204, 0, 0.7);\n";
//     echo "}\n";
//     echo ".error-zone:hover {\n";
//     echo "    transform: rotateY(0deg) rotateX(0deg) translateZ(80px);\n";
//     echo "    box-shadow: 0 0 40px #ff3333, 0 0 60px #ff6666;\n";
//     echo "}\n";
//     echo "h2 {\n";
//     echo "    font-size: 28px;\n";
//     echo "    margin-bottom: 20px;\n";
//     echo "    text-shadow: 0 0 15px #ff3333, 0 0 30px #ff6666;\n";
//     echo "    animation: pulseText 2s infinite;\n";
//     echo "}\n";
//     echo "p {\n";
//     echo "    font-size: 18px;\n";
//     echo "    line-height: 1.6;\n";
//     echo "    text-shadow: 0 0 10px #ff3333;\n";
//     echo "    animation: textPulse 2s infinite ease-in-out;\n";
//     echo "}\n";
//     echo "a {\n";
//     echo "    color: #ffcc00;\n";
//     echo "    background: rgba(0, 0, 0, 0.5);\n";
//     echo "    border: 3px solid #ffcc00;\n";
//     echo "    padding: 12px 25px;\n";
//     echo "    border-radius: 15px;\n";
//     echo "    text-decoration: none;\n";
//     echo "    font-weight: bold;\n";
//     echo "    cursor: pointer;\n";
//     echo "    transition: all 0.4s;\n";
//     echo "    display: inline-block;\n";
//     echo "    text-align: center;\n";
//     echo "    margin: 10px;\n";
//     echo "    font-size: 18px;\n";
//     echo "    transform: translateZ(20px);\n";
//     echo "}\n";
//     echo "a:hover {\n";
//     echo "    background: #ff3333;\n";
//     echo "    color: #ffcc00;\n";
//     echo "    transform: scale(1.15) rotateX(15deg) translateZ(40px);\n";
//     echo "    box-shadow: 0 0 25px #ffcc00, 0 0 50px #ff6666;\n";
//     echo "}\n";
//     echo ".bg-anim {\n";
//     echo "    position: absolute;\n";
//     echo "    background: rgba(255, 51, 0, 0.7);\n";
//     echo "    border-radius: 50%;\n";
//     echo "    animation: floatJazz 5s infinite ease-in-out;\n";
//     echo "    z-index: 0;\n";
//     echo "    box-shadow: 0 0 20px #ffcc00;\n";
//     echo "}\n";
//     echo ".bg-anim:nth-child(1) { width: 120px; height: 120px; top: 15%; left: 10%; animation-delay: 0s; }\n";
//     echo ".bg-anim:nth-child(2) { width: 90px; height: 90px; top: 30%; right: 15%; animation-delay: 1s; }\n";
//     echo ".bg-anim:nth-child(3) { width: 140px; height: 140px; bottom: 25%; left: 20%; animation-delay: 2s; }\n";
//     echo ".bg-anim:nth-child(4) { width: 80px; height: 80px; bottom: 10%; right: 20%; animation-delay: 3s; }\n";
//     echo ".bg-anim:nth-child(5) { width: 110px; height: 110px; top: 40%; left: 45%; animation-delay: 4s; }\n";
//     echo "@keyframes jazzBackground {\n";
//     echo "    0% { background-position: 0% 0%; }\n";
//     echo "    25% { background-position: 50% 50%; }\n";
//     echo "    50% { background-position: 100% 100%; }\n";
//     echo "    75% { background-position: 50% 50%; }\n";
//     echo "    100% { background-position: 0% 0%; }\n";
//     echo "}\n";
//     echo "@keyframes glowJazz {\n";
//     echo "    0% { transform: translateZ(50px) scale(1); text-shadow: 0 0 25px #ff3333, 0 0 50px #ff6666; }\n";
//     echo "    100% { transform: translateZ(70px) scale(1.1); text-shadow: 0 0 40px #ff3333, 0 0 80px #ff6666; }\n";
//     echo "}\n";
//     echo "@keyframes pulseText {\n";
//     echo "    0% { transform: scale(1); }\n";
//     echo "    50% { transform: scale(1.05); }\n";
//     echo "    100% { transform: scale(1); }\n";
//     echo "}\n";
//     echo "@keyframes floatJazz {\n";
//     echo "    0% { transform: translateZ(0) rotate(0deg); }\n";
//     echo "    50% { transform: translateZ(120px) rotate(180deg); }\n";
//     echo "    100% { transform: translateZ(0) rotate(360deg); }\n";
//     echo "}\n";
//     echo "@keyframes textPulse {\n";
//     echo "    0% { transform: scale(1); }\n";
//     echo "    50% { transform: scale(1.02); }\n";
//     echo "    100% { transform: scale(1); }\n";
//     echo "}\n";
//     echo "</style>\n";
//     echo "</head>\n";
//     echo "<body>\n";
//     echo "<div class=\"bg-anim\"></div>\n";
//     echo "<div class=\"bg-anim\"></div>\n";
//     echo "<div class=\"bg-anim\"></div>\n";
//     echo "<div class=\"bg-anim\"></div>\n";
//     echo "<div class=\"bg-anim\"></div>\n";
//     echo "<h1>405 - Method Not Allowed</h1>\n";
//     echo "<div class=\"container\">\n";
//     echo "<div class=\"error-zone\">\n";
//     echo "<h2>Wrong Move, Groove!</h2>\n";
//     echo "<p>The beat dropped—your request method isn’t in our playlist!</p>\n";
//     echo "<a href=\"/\">BACK TO THE STAGE</a>\n";
//     echo "</div>\n";
//     echo "</div>\n";
//     echo "</body>\n";
//     echo "</html>\n";
//     exit(0);
// }

// Check protocol version (Option with redirect)
$protocol = getenv("SERVER_PROTOCOL");
if ($protocol !== "HTTP/1.1") {
    header("Status: 505 HTTP Version Not Supported");
    //header("Location: /505.html");
    $errorFile = $_SERVER['DOCUMENT_ROOT'] . "/errors/505.html";
    if (file_exists($errorFile)) {
        header("Content-type: text/html; charset=UTF-8");
        readfile($errorFile);
    } else {
        echo "<html><body><h1>505 HTTP Version Not Supported</h1>";
        echo "<p>This server requires HTTP/1.1 or higher.</p></body></html>";
    }
    exit(0);
}

// Check protocol version (ALTERNATIVE WAY: MAKE PAGE ON THE GO)
// $protocol = getenv("SERVER_PROTOCOL");
// if ($protocol !== "HTTP/1.1") {
//     send_error(505, "HTTP Version Not Supported");
// }

// Check content length
$content_length = (int) getenv("CONTENT_LENGTH");
if ($content_length > 0) {
    send_error(400, "Bad Request: Fortunes Don't Need Extra Baggage!");
}

// Trigger only 418 error with original probability
$randomValue = rand(1, 100);
if ($randomValue < 5) { // 5% chance for 418 error
   send_error(418, "I'M A TEAPOT! I Cannot Brew Coffee! ☕");
}

// Simulate 418, 503 and 504 errors
// $randomValue = rand(1, 100);
// if ($randomValue < 5) {
//     send_error(418, "I'M A TEAPOT! I Cannot Brew Coffee! ☕");
// } elseif ($randomValue < 10) {
//     send_error(503, "Service Unavailable");
// } elseif ($randomValue < 15) {
//     send_error(504, "Gateway Timeout");
// }

// Array of fortune cookie messages
$fortunes = [
    "You will find great success in your near future. How near? Not sure.",
    "A pleasant surprise is waiting for you. Somewhere. Somehow. That's all I know, OK?",
    "Your talents will be recognized and rewarded. One day...",
    "Good things come to those who wait patiently. So wait. Just wait...",
    "A new adventure awaits you around the corner. :)",
    "Wisdom will guide you to make the right choices. It's a promise.",
    "Happiness is not a destination, it's a journey. Don't put your fate in fortune cookies.",
    "Your creativity will lead you to new opportunities. Nice.",
    "A friend will bring you unexpected joy soon. How cool.",
    "The best is yet to come - stay optimistic!",
    "You will soon pass a very cool version of Webserv and mark it as outstanding! Yay!",
    "No idea. The future is open.",
    "You will smoke weed soon.",
    "A big rock may try to catch you in the following days. Don't ask why...",
    "If you fail this evaluation, you'll get community service real soon. And your pillow will always be wet. I don't call the shots...",
    "Tomorrow you will have the choice to give a smile to a stranger. That's it.",
    "Tomorrow you will have the choice to give without wanting anything.",
    "Life is beautiful and you are soon going to embrace that.",
    "You are gonna be emotional.",
    "Stay away from dreams that are too big and know that whatever awaits for you is good.",
    "Start walking the way and the way will show itself."
];

// Get a random fortune
try {
    $randomFortune = $fortunes[array_rand($fortunes)];
} catch (Exception $e) {
    header("Status: 500 Internal Server Error");
    echo "<!DOCTYPE html>\n";
    echo "<html lang=\"en\">\n";
    echo "<head>\n";
    echo "<meta charset=\"UTF-8\">\n";
    echo "<title>Webserv - 500 Internal Server Error: Fortune Machine Broke Down!</title>\n";
    echo "<style>\n";
    echo "body {\n";
    echo "    margin: 0;\n";
    echo "    font-family: 'Courier New', monospace;\n";
    echo "    background: linear-gradient(135deg, #cc0000, #ff3333, #990000, #ff6666, #cc0000);\n";
    echo "    background-size: 500% 500%;\n";
    echo "    animation: jazzBackground 8s ease-in-out infinite;\n";
    echo "    color: #ffcc00;\n";
    echo "    perspective: 1200px;\n";
    echo "    overflow-x: hidden;\n";
    echo "    position: relative;\n";
    echo "    min-height: 100vh;\n";
    echo "}\n";
    echo ".container {\n";
    echo "    display: flex;\n";
    echo "    flex-direction: column;\n";
    echo "    align-items: center;\n";
    echo "    padding: 20px;\n";
    echo "    position: relative;\n";
    echo "    z-index: 1;\n";
    echo "    transform-style: preserve-3d;\n";
    echo "}\n";
    echo "h1 {\n";
    echo "    font-size: 72px;\n";
    echo "    text-transform: uppercase;\n";
    echo "    letter-spacing: 10px;\n";
    echo "    text-align: center;\n";
    echo "    animation: glowJazz 1.5s infinite alternate;\n";
    echo "    text-shadow: 0 0 25px #ff3333, 0 0 50px #ff6666, 0 0 75px #cc0000;\n";
    echo "    margin-top: 50px;\n";
    echo "    margin-bottom: 20px;\n";
    echo "    transform: translateZ(50px) rotateX(10deg);\n";
    echo "    position: relative;\n";
    echo "    z-index: 2;\n";
    echo "}\n";
    echo ".error-zone {\n";
    echo "    background: rgba(255, 51, 0, 0.85);\n";
    echo "    padding: 25px;\n";
    echo "    border: 4px solid #ffcc00;\n";
    echo "    border-radius: 25px;\n";
    echo "    width: 85%;\n";
    echo "    max-width: 700px;\n";
    echo "    margin-bottom: 30px;\n";
    echo "    transform: rotateY(-15deg) rotateX(-5deg);\n";
    echo "    transition: transform 0.6s, box-shadow 0.6s;\n";
    echo "    box-shadow: 0 0 20px rgba(255, 204, 0, 0.7);\n";
    echo "}\n";
    echo ".error-zone:hover {\n";
    echo "    transform: rotateY(0deg) rotateX(0deg) translateZ(80px);\n";
    echo "    box-shadow: 0 0 40px #ff3333, 0 0 60px #ff6666;\n";
    echo "}\n";
    echo "h2 {\n";
    echo "    font-size: 28px;\n";
    echo "    margin-bottom: 20px;\n";
    echo "    text-shadow: 0 0 15px #ff3333, 0 0 30px #ff6666;\n";
    echo "    animation: pulseText 2s infinite;\n";
    echo "}\n";
    echo "p {\n";
    echo "    font-size: 18px;\n";
    echo "    line-height: 1.6;\n";
    echo "    text-shadow: 0 0 10px #ff3333;\n";
    echo "    animation: textPulse 2s infinite ease-in-out;\n";
    echo "}\n";
    echo "a {\n";
    echo "    color: #ffcc00;\n";
    echo "    background: rgba(0, 0, 0, 0.5);\n";
    echo "    border: 3px solid #ffcc00;\n";
    echo "    padding: 12px 25px;\n";
    echo "    border-radius: 15px;\n";
    echo "    text-decoration: none;\n";
    echo "    font-weight: bold;\n";
    echo "    cursor: pointer;\n";
    echo "    transition: all 0.4s;\n";
    echo "    display: inline-block;\n";
    echo "    text-align: center;\n";
    echo "    margin: 10px;\n";
    echo "    font-size: 18px;\n";
    echo "    transform: translateZ(20px);\n";
    echo "}\n";
    echo "a:hover {\n";
    echo "    background: #ff3333;\n";
    echo "    color: #ffcc00;\n";
    echo "    transform: scale(1.15) rotateX(15deg) translateZ(40px);\n";
    echo "    box-shadow: 0 0 25px #ffcc00, 0 0 50px #ff6666;\n";
    echo "}\n";
    echo ".bg-anim {\n";
    echo "    position: absolute;\n";
    echo "    background: rgba(255, 51, 0, 0.7);\n";
    echo "    border-radius: 50%;\n";
    echo "    animation: floatJazz 5s infinite ease-in-out;\n";
    echo "    z-index: 0;\n";
    echo "    box-shadow: 0 0 20px #ffcc00;\n";
    echo "}\n";
    echo ".bg-anim:nth-child(1) { width: 120px; height: 120px; top: 15%; left: 10%; animation-delay: 0s; }\n";
    echo ".bg-anim:nth-child(2) { width: 90px; height: 90px; top: 30%; right: 15%; animation-delay: 1s; }\n";
    echo ".bg-anim:nth-child(3) { width: 140px; height: 140px; bottom: 25%; left: 20%; animation-delay: 2s; }\n";
    echo ".bg-anim:nth-child(4) { width: 80px; height: 80px; bottom: 10%; right: 20%; animation-delay: 3s; }\n";
    echo ".bg-anim:nth-child(5) { width: 110px; height: 110px; top: 40%; left: 45%; animation-delay: 4s; }\n";
    echo "@keyframes jazzBackground {\n";
    echo "    0% { background-position: 0% 0%; }\n";
    echo "    25% { background-position: 50% 50%; }\n";
    echo "    50% { background-position: 100% 100%; }\n";
    echo "    75% { background-position: 50% 50%; }\n";
    echo "    100% { background-position: 0% 0%; }\n";
    echo "}\n";
    echo "@keyframes glowJazz {\n";
    echo "    0% { transform: translateZ(50px) scale(1); text-shadow: 0 0 25px #ff3333, 0 0 50px #ff6666; }\n";
    echo "    100% { transform: translateZ(70px) scale(1.1); text-shadow: 0 0 40px #ff3333, 0 0 80px #ff6666; }\n";
    echo "}\n";
    echo "@keyframes pulseText {\n";
    echo "    0% { transform: scale(1); }\n";
    echo "    50% { transform: scale(1.05); }\n";
    echo "    100% { transform: scale(1); }\n";
    echo "}\n";
    echo "@keyframes floatJazz {\n";
    echo "    0% { transform: translateZ(0) rotate(0deg); }\n";
    echo "    50% { transform: translateZ(120px) rotate(180deg); }\n";
    echo "    100% { transform: translateZ(0) rotate(360deg); }\n";
    echo "}\n";
    echo "@keyframes textPulse {\n";
    echo "    0% { transform: scale(1); }\n";
    echo "    50% { transform: scale(1.02); }\n";
    echo "    100% { transform: scale(1); }\n";
    echo "}\n";
    echo "</style>\n";
    echo "</head>\n";
    echo "<body>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<div class=\"bg-anim\"></div>\n";
    echo "<h1>500 - Internal Server Error</h1>\n";
    echo "<div class=\"container\">\n";
    echo "<div class=\"error-zone\">\n";
    echo "<h2>Server Solo Crashed!</h2>\n";
    echo "<p>Oof—the Webserv crew hit an unexpected riff! Give us a sec to tune up.</p>\n";
    echo "<a href=\"/\">RESTART THE SHOW</a>\n";
    echo "</div>\n";
    echo "</div>\n";
    echo "<script>\n";
    echo "    console.error('Error: Fortune response status: 500');\n";
    echo "</script>\n";
    echo "</body>\n";
    echo "</html>\n";
    exit(0);
}

// HTML output for success
$statusCode = 200;
header("Status: 200 OK");
header("Content-Type: text/html");
echo "<!DOCTYPE html>\n";
echo "<html lang=\"en\">\n";
echo "<head>\n";
echo "<meta charset=\"UTF-8\">\n";
echo "<title>Webserv - Your Fortune</title>\n";
echo "<style>\n";
echo "body {\n";
echo "    margin: 0;\n";
echo "    font-family: 'Courier New', monospace;\n";
echo "    background: linear-gradient(90deg, #cc0000, #ff3333, #cc0000);\n";
echo "    background-size: 300% 300%;\n";
echo "    animation: redGlow 6s ease-in-out infinite;\n";
echo "    color: #ffcc00;\n";
echo "    perspective: 800px;\n";
echo "    overflow-x: hidden;\n";
echo "    position: relative;\n";
echo "    min-height: 100vh;\n";
echo "}\n";
echo ".container {\n";
echo "    display: flex;\n";
echo "    flex-direction: column;\n";
echo "    align-items: center;\n";
echo "    padding: 40px;\n";
echo "    position: relative;\n";
echo "    z-index: 1;\n";
echo "    transform-style: preserve-3d;\n";
echo "}\n";
echo "h1 {\n";
echo "    font-size: 60px;\n";
echo "    text-transform: uppercase;\n";
echo "    letter-spacing: 8px;\n";
echo "    text-align: center;\n";
echo "    animation: titleBounce 2s infinite ease-in-out;\n";
echo "    text-shadow: 0 0 20px #ff3333, 0 0 40px #cc0000;\n";
echo "    margin-top: 50px;\n";
echo "    margin-bottom: 20px;\n";
echo "    transform: translateZ(30px);\n";
echo "    position: relative;\n";
echo "    z-index: 2;\n";
echo "}\n";
echo "p {\n";
echo "    font-size: 26px;\n";
echo "    font-weight: 900;\n";
echo "    -webkit-text-stroke: 1px #ffcc00;\n";
echo "    line-height: 1.6;\n";
echo "    text-shadow: 0 0 15px #ff3333, 0 0 25px #cc0000;\n";
echo "    animation: oracleGlow 3s infinite ease-in-out;\n";
echo "    margin: 20px;\n";
echo "    transform: translateZ(20px);\n";
echo "    text-align: center;\n";
echo "}\n";
echo "a {\n";
echo "    color: #ffcc00;\n";
echo "    background: rgba(255, 51, 0, 0.5);\n";
echo "    border: 3px solid #ffcc00;\n";
echo "    padding: 12px 25px;\n";
echo "    border-radius: 15px;\n";
echo "    text-decoration: none;\n";
echo "    font-weight: bold;\n";
echo "    cursor: pointer;\n";
echo "    transition: all 0.4s;\n";
echo "    display: inline-block;\n";
echo "    text-align: center;\n";
echo "    margin: 15px;\n";
echo "    font-size: 18px;\n";
echo "    transform: translateZ(20px);\n";
echo "}\n";
echo "a:hover {\n";
echo "    background: #ffcc00;\n";
echo "    color: #cc0000;\n";
echo "    transform: scale(1.1) translateZ(30px);\n";
echo "    box-shadow: 0 0 20px #ffcc00;\n";
echo "}\n";
echo ".bg-anim {\n";
echo "    position: absolute;\n";
echo "    background: rgba(255, 51, 0, 0.7);\n";
echo "    border-radius: 50%;\n";
echo "    animation: floatJazz 5s infinite ease-in-out;\n";
echo "    z-index: 0;\n";
echo "    box-shadow: 0 0 20px #ffcc00;\n";
echo "}\n";
echo ".bg-anim:nth-child(1) { width: 120px; height: 120px; top: 15%; left: 10%; animation-delay: 0s; }\n";
echo ".bg-anim:nth-child(2) { width: 90px; height: 90px; top: 30%; right: 15%; animation-delay: 1s; }\n";
echo ".bg-anim:nth-child(3) { width: 140px; height: 140px; bottom: 25%; left: 20%; animation-delay: 2s; }\n";
echo ".bg-anim:nth-child(4) { width: 80px; height: 80px; bottom: 10%; right: 20%; animation-delay: 3s; }\n";
echo ".bg-anim:nth-child(5) { width: 110px; height: 110px; top: 40%; left: 45%; animation-delay: 4s; }\n";
echo ".fortune-orb {\n";
echo "    position: absolute;\n";
echo "    bottom: 0;\n";
echo "    left: 50%;\n";
echo "    transform: translateX(-50%);\n";
echo "    background: rgba(255, 51, 0, 0.8);\n";
echo "    border: 3px solid #ffcc00;\n";
echo "    border-radius: 50%;\n";
echo "    width: 100px;\n";
echo "    height: 100px;\n";
echo "    animation: orbFloat 4s infinite ease-in-out;\n";
echo "    z-index: 0;\n";
echo "    box-shadow: 0 0 20px #ffcc00;\n";
echo "    display: flex;\n";
echo "    align-items: center;\n";
echo "    justify-content: center;\n";
echo "    font-size: 14px;\n";
echo "    text-align: center;\n";
echo "    text-shadow: 0 0 10px #ff3333;\n";
echo "}\n";
echo "@keyframes redGlow {\n";
echo "    0% { background-position: 0% 50%; }\n";
echo "    50% { background-position: 100% 50%; }\n";
echo "    100% { background-position: 0% 50%; }\n";
echo "}\n";
echo "@keyframes titleBounce {\n";
echo "    0% { transform: translateZ(30px) scale(1); }\n";
echo "    50% { transform: translateZ(40px) scale(1.05); }\n";
echo "    100% { transform: translateZ(30px) scale(1); }\n";
echo "}\n";
echo "@keyframes oracleGlow {\n";
echo "    0% { text-shadow: 0 0 15px #ff3333, 0 0 25px #cc0000; transform: translateZ(20px) scale(1); }\n";
echo "    50% { text-shadow: 0 0 25px #ff3333, 0 0 35px #cc0000; transform: translateZ(25px) scale(1.02); }\n";
echo "    100% { text-shadow: 0 0 15px #ff3333, 0 0 25px #cc0000; transform: translateZ(20px) scale(1); }\n";
echo "}\n";
echo "@keyframes floatJazz {\n";
echo "    0% { transform: translateZ(0) rotate(0deg); }\n";
echo "    50% { transform: translateZ(120px) rotate(180deg); }\n";
echo "    100% { transform: translateZ(0) rotate(360deg); }\n";
echo "}\n";
echo "@keyframes orbFloat {\n";
echo "    0% { transform: translateX(-50%) translateY(0); }\n";
echo "    50% { transform: translateX(-50%) translateY(-50px); }\n";
echo "    100% { transform: translateX(-50%) translateY(0); }\n";
echo "}\n";
echo "</style>\n";
echo "</head>\n";
echo "<body>\n";
echo "<div class=\"bg-anim\"></div>\n";
echo "<div class=\"bg-anim\"></div>\n";
echo "<div class=\"bg-anim\"></div>\n";
echo "<div class=\"bg-anim\"></div>\n";
echo "<div class=\"bg-anim\"></div>\n";
echo "<div class=\"fortune-orb\">Destiny Awaits</div>\n";
echo "<div class=\"container\">\n";
echo "<h1>Your Fortune</h1>\n";
echo "<p>" . htmlspecialchars($randomFortune) . "</p>\n";
echo "<a href=\"/cgi-bin/fortune.php\">Another Fortune</a>\n";
echo "<a href=\"/\">Back to Main Page</a>\n";
echo "</div>\n";
echo "<script>\n";
echo "    console.info('Fortune loaded successfully, status: 200, content-type: text/html');\n";
echo "</script>\n";
echo "</body>\n";
echo "</html>\n";
?>
