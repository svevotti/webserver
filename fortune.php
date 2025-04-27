<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Webserv - Your Fortune</title>
<style>
body {
    margin: 0;
    font-family: 'Courier New', monospace;
    background: linear-gradient(90deg, #cc0000, #ff3333, #cc0000);
    background-size: 300% 300%;
    animation: redGlow 6s ease-in-out infinite;
    color: #ffcc00;
    perspective: 800px;
    overflow-x: hidden;
    position: relative;
    min-height: 100vh;
}
.container {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 40px;
    position: relative;
    z-index: 1;
    transform-style: preserve-3d;
}
h1 {
    font-size: 60px;
    text-transform: uppercase;
    letter-spacing: 8px;
    text-align: center;
    animation: titleBounce 2s infinite ease-in-out;
    text-shadow: 0 0 20px #ff3333, 0 0 40px #cc0000;
    margin-top: 50px;
    margin-bottom: 20px;
    transform: translateZ(30px);
    position: relative;
    z-index: 2;
}
p {
    font-size: 26px;
    font-weight: 900;
    -webkit-text-stroke: 1px #ffcc00;
    line-height: 1.6;
    text-shadow: 0 0 15px #ff3333, 0 0 25px #cc0000;
    animation: oracleGlow 3s infinite ease-in-out;
    margin: 20px;
    transform: translateZ(20px);
    text-align: center;
}
a {
    color: #ffcc00;
    background: rgba(255, 51, 0, 0.5);
    border: 3px solid #ffcc00;
    padding: 12px 25px;
    border-radius: 15px;
    text-decoration: none;
    font-weight: bold;
    cursor: pointer;
    transition: all 0.4s;
    display: inline-block;
    text-align: center;
    margin: 15px;
    font-size: 18px;
    transform: translateZ(20px);
}
a:hover {
    background: #ffcc00;
    color: #cc0000;
    transform: scale(1.1) translateZ(30px);
    box-shadow: 0 0 20px #ffcc00;
}
.bg-anim {
    position: absolute;
    background: rgba(255, 51, 0, 0.7);
    border-radius: 50%;
    animation: floatJazz 5s infinite ease-in-out;
    z-index: 0;
    box-shadow: 0 0 20px #ffcc00;
}
.bg-anim:nth-child(1) { width: 120px; height: 120px; top: 15%; left: 10%; animation-delay: 0s; }
.bg-anim:nth-child(2) { width: 90px; height: 90px; top: 30%; right: 15%; animation-delay: 1s; }
.bg-anim:nth-child(3) { width: 140px; height: 140px; bottom: 25%; left: 20%; animation-delay: 2s; }
.bg-anim:nth-child(4) { width: 80px; height: 80px; bottom: 10%; right: 20%; animation-delay: 3s; }
.bg-anim:nth-child(5) { width: 110px; height: 110px; top: 40%; left: 45%; animation-delay: 4s; }
.fortune-orb {
    position: absolute;
    bottom: 0;
    left: 50%;
    transform: translateX(-50%);
    background: rgba(255, 51, 0, 0.8);
    border: 3px solid #ffcc00;
    border-radius: 50%;
    width: 100px;
    height: 100px;
    animation: orbFloat 4s infinite ease-in-out;
    z-index: 0;
    box-shadow: 0 0 20px #ffcc00;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 14px;
    text-align: center;
    text-shadow: 0 0 10px #ff3333;
}
@keyframes redGlow {
    0% { background-position: 0% 50%; }
    50% { background-position: 100% 50%; }
    100% { background-position: 0% 50%; }
}
@keyframes titleBounce {
    0% { transform: translateZ(30px) scale(1); }
    50% { transform: translateZ(40px) scale(1.05); }
    100% { transform: translateZ(30px) scale(1); }
}
@keyframes oracleGlow {
    0% { text-shadow: 0 0 15px #ff3333, 0 0 25px #cc0000; transform: translateZ(20px) scale(1); }
    50% { text-shadow: 0 0 25px #ff3333, 0 0 35px #cc0000; transform: translateZ(25px) scale(1.02); }
    100% { text-shadow: 0 0 15px #ff3333, 0 0 25px #cc0000; transform: translateZ(20px) scale(1); }
}
@keyframes floatJazz {
    0% { transform: translateZ(0) rotate(0deg); }
    50% { transform: translateZ(120px) rotate(180deg); }
    100% { transform: translateZ(0) rotate(360deg); }
}
@keyframes orbFloat {
    0% { transform: translateX(-50%) translateY(0); }
    50% { transform: translateX(-50%) translateY(-50px); }
    100% { transform: translateX(-50%) translateY(0); }
}
</style>
</head>
<body>
<div class="bg-anim"></div>
<div class="bg-anim"></div>
<div class="bg-anim"></div>
<div class="bg-anim"></div>
<div class="bg-anim"></div>
<div class="fortune-orb">Destiny Awaits</div>
<div class="container">
<h1>Your Fortune</h1>
<p>A pleasant surprise is waiting for you. Somewhere. Somehow. That&#039;s all I know, OK?</p>
<a href="/cgi-bin/fortune.php">Another Fortune</a>
<a href="/">Back to Main Page</a>
</div>
<script>
    console.info('Fortune loaded successfully, status: 200, content-type: text/html');
</script>
</body>
</html>
