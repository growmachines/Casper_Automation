
// Trigger browser time send on page load
document.addEventListener('DOMContentLoaded', sendBrowserTime);
async function fetchAndDisplayTime() {
    try {
        const response = await fetch('/GetSystemTime');
        if (response.ok) {
            const { time, utcOffset } = await response.json();
            const timeDisplay = document.getElementById('time-display');
            if (timeDisplay) {
                timeDisplay.textContent = `System Time: ${time} UTC Offset: ${utcOffset}`;
            }
        } else {
            console.warn("Failed to fetch system time.");
        }
    } catch (error) {
        console.error("Error fetching system time:", error);
    }
}

// Fetch and display time on page load
document.addEventListener('DOMContentLoaded', fetchAndDisplayTime);
async function fetchAndDisplayTime() {
    try {
        const response = await fetch('/GetSystemTime');
        if (response.ok) {
            const { time, utcOffset } = await response.json();
            const timeDisplay = document.getElementById('time-display');
            if (timeDisplay) {
                timeDisplay.textContent = `System Time: ${time} UTC Offset: ${utcOffset}`;
            }
        } else {
            console.warn("Failed to fetch system time.");
        }
    } catch (error) {
        console.error("Error fetching system time:", error);
    }
}

// Fetch and display time on page load
document.addEventListener('DOMContentLoaded', fetchAndDisplayTime);
function startPeriodicTimeUpdate(interval = 10000) {
    setInterval(fetchAndDisplayTime, interval);
}

// Start periodic updates on page load
document.addEventListener('DOMContentLoaded', () => {
    startPeriodicTimeUpdate();
});
async function sendBrowserTime() {
    const now = new Date();
    now.setHours(now.getHours() + 2); // Adjust for UTC+2
    const timeString = now.toISOString().split('.')[0]; // Format: YYYY-MM-DDTHH:MM:SS
    const offset = "+02:00"; // Static for now, can be dynamic later

    try {
        const response = await fetch('/SetBrowserTime', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `time=${encodeURIComponent(timeString)}&utcOffset=${encodeURIComponent(offset)}`
        });

        if (response.ok) {
            console.log("Browser time sent successfully.");
        } else {
            console.error("Failed to send browser time.");
        }
    } catch (error) {
        console.error("Error sending browser time:", error);
    }
}
