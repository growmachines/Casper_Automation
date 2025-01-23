document.addEventListener('DOMContentLoaded', () => {
    console.log('Initializing AP Settings Page...');

    // Common elements  
    const ssidDropdown = document.getElementById('ssid');
    const saveCredentialsButton = document.getElementById('saveCredentialsButton');
    const alternativeIpField = document.getElementById('alternative-ip');
    const WiFi_Password = document.getElementById('WiFi_password'); 
    const factoryResetButton = document.getElementById('factory-reset');

    // Utility to validate IP address
    const isValidIP = (ip) => {
        const ipPattern = /^\d{1,3}(\.\d{1,3}){3}$/;
        return ipPattern.test(ip) && ip.split('.').every(num => parseInt(num, 10) >= 0 && parseInt(num, 10) <= 255);
    };

    // Fetch and populate available networks
    async function fetchNetworks() {
        try {
            const response = await fetch('/WiFiScan');
            const networks = await response.json();

            ssidDropdown.innerHTML = '<option value="">Select a Network</option>';
            networks.forEach(ssid => {
                const option = document.createElement('option');
                option.value = ssid;
                option.textContent = ssid;
                ssidDropdown.appendChild(option);
            });
        } catch (error) {
            console.error('Error fetching networks:', error);
            ssidDropdown.innerHTML = '<option value="">Error loading networks</option>';
        }
    }

    // Factory reset functionality
    factoryResetButton.addEventListener('click', () => {
        if (confirm('Are you sure you want to reset to factory settings? This action cannot be undone.')) {
            fetch('/FactoryReset', { method: 'POST' })
                .then(response => response.text())
                .then(result => alert(result))
                .catch(error => alert('Error resetting settings: ' + error));
        }
    });

    // Save credentials and connect to Wi-Fi
    saveCredentialsButton.addEventListener('click', async () => {
        const ssid = ssidDropdown.value;
        const WiFiPassword = WiFi_Password.value;
        const alternativeIP = alternativeIpField.value;

        if (!ssid) {
            alert('Please select a Wi-Fi network.');
            return;
        }

        if (alternativeIP && !isValidIP(alternativeIP)) {
            alert('Please enter a valid IP address.');
            return;
        }

        try {
            saveCredentialsButton.disabled = true;
            saveCredentialsButton.textContent = "Connecting...";

            const body = `ssid=${encodeURIComponent(ssid)}&WiFiPassword=${encodeURIComponent(WiFiPassword)}&alternativeIP=${encodeURIComponent(alternativeIP)}`;

            const response = await fetch('/SaveAPConfig', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body,
            });

            const message = await response.text();
            if (response.ok) {
                alert(message);
                location.reload();
            } else {
                alert(`Failed to save credentials: ${message}`);
            }
        } catch (error) {
            console.error('Error saving credentials:', error);
            alert('An error occurred while saving credentials. Please try again.');
        } finally {
            saveCredentialsButton.disabled = false;
            saveCredentialsButton.textContent = "Save Credentials and Connect to Wi-Fi Network";
        }
    });

    // Initialize on page load
    fetchNetworks();
});
