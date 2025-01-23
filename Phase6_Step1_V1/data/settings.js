iconPreview: document.getElementById('icon-preview'),
// Revised JavaScript for Settings Page Integration with Backend

    async function fetchModeAndUpdateHeader() {
        try {
            // Fetch mode and SSID dynamically from the backend
            const response = await fetch('/CurrentMode'); // Endpoint must return { mode: 'AP'/'STA', ssid: 'SSID' }
            const { mode, ssid } = await response.json();

            // Update header dynamically based on the mode
            if (mode === 'AP') {
                headerTitle.textContent = 'Settings (Access Point Mode)';
                connectionStatus.textContent = '(Connected to Casper_Automation)';
            } else if (mode === 'STA') {
                headerTitle.textContent = 'Settings (Station Mode)';
                connectionStatus.textContent = ssid ? `(${ssid})` : '(Not connected)';
            }
        } catch (error) {
            console.error('Error fetching mode information:', error);
            headerTitle.textContent = 'Settings (Unknown Mode)';
            connectionStatus.textContent = '(Error fetching mode)';
        }
    }

    // Call the function to update the header dynamically
    fetchModeAndUpdateHeader();

 // Update Header Dynamically Based on Mode
   
	const headerTitle = document.getElementById('header-title');

    const connectionStatus = document.getElementById('connection-status');

    async function fetchModeAndUpdateHeader() {
        try {
            // Fetch mode and SSID dynamically from the backend
            const response = await fetch('/CurrentMode'); // Endpoint must return { mode: 'AP'/'STA', ssid: 'SSID' }
            const { mode, ssid } = await response.json();

            // Update header dynamically based on the mode
            if (mode === 'AP') {
                headerTitle.textContent = 'Settings (Access Point Mode)';
                connectionStatus.textContent = '(Connected to Casper_Automation)';
            } else if (mode === 'STA') {
                headerTitle.textContent = 'Settings (Station Mode)';
                connectionStatus.textContent = ssid ? `(${ssid})` : '(Not connected)';
            }
        } catch (error) {
            console.error('Error fetching mode information:', error);
            headerTitle.textContent = 'Settings (Unknown Mode)';
            connectionStatus.textContent = '(Error fetching mode)';
        }
    }

    // Call the function to update the header dynamically
    fetchModeAndUpdateHeader();
// Shared Elements
const sharedElements = {
    iconDropdown: document.getElementById('icon-dropdown'),
    iconPreview: document.getElementById('icon-preview'),
    timeDisplay: document.getElementById('time-display'),
    switchSelect: document.getElementById('switch-select'),
    saveSettingsButton: document.getElementById('save-sta-settings'),
    clearTimersButton: document.getElementById('clear-timers'),
    usernameInput: document.getElementById('username'),
    passwordInput: document.getElementById('password'),
    togglePasswordIcon: document.getElementById('toggle-password'),
    saveCredentialsButton: document.getElementById('saveCredentialsButton'),
    factoryResetButton: document.getElementById('factory-reset'),
    ssidDropdown: document.getElementById('ssidDropdown'),
    alternativeIpField: document.getElementById('alternative-ip'),
    WiFiPassword: document.getElementById('WiFi_password'),
	toggleWiFiPasswordIcon: document.getElementById('toggle-wifi-password'),
    headerTitle: document.getElementById('header-title'),
    connectionStatus: document.getElementById('connection-status'),
};

// Utility Functions// Utility Functions
async function fetchFromAPI(endpoint, options = {}) {
    try {
        const response = await fetch(endpoint, options);
        if (!response.ok) throw new Error(`Failed to fetch from ${endpoint}`);
        return await response.json();
    } catch (error) {
        console.error(`Error fetching from ${endpoint}:`, error);
        throw error;
    }
}

// Handle Icon Selection
sharedElements.iconDropdown?.addEventListener('change', () => {
    const selectedIcon = sharedElements.iconDropdown.value; // Get the selected value
    setSelectedIcon(selectedIcon); // Update icon preview
});


// Toggle Password Visibility for Wi-Fi Password
function toggleWiFiPasswordVisibility() {
    const passwordField = document.getElementById('WiFi_password');
    const toggleIcon = document.getElementById('toggle-wifi-password');

    if (!passwordField || !toggleIcon) {
        console.error('Password field or toggle icon not found.');
        return;
    }

    toggleIcon.addEventListener('click', () => {
        const isPasswordVisible = passwordField.type === 'text';
        passwordField.type = isPasswordVisible ? 'password' : 'text';
        toggleIcon.classList.toggle('fa-eye', isPasswordVisible);
        toggleIcon.classList.toggle('fa-eye-slash', !isPasswordVisible);
    });
}






// Retry mechanism for fetching icons
const fetchIconsWithRetry = (retries = 3, delay = 500) => {
    return new Promise((resolve, reject) => {
        const attemptFetch = (remainingRetries) => {
            console.log(`Fetching icons, retries left: ${remainingRetries}`);
            fetch('/list-icons')
                .then(response => {
                    if (!response.ok) throw new Error('Failed to fetch icons');
                    return response.json();
                })
                .then(data => {
                    if (Array.isArray(data) && data.length > 0) {
                        resolve(data); // Return fetched icons
                    } else {
                        throw new Error('No icons returned');
                    }
                })
                .catch(error => {
                    console.error('Error fetching icons:', error);
                    if (remainingRetries > 0) {
                        setTimeout(() => attemptFetch(remainingRetries - 1), delay);
                    } else {
                        reject('Failed to fetch icons after multiple attempts');
                    }
                });
        };
        attemptFetch(retries);
    });
};

// Populate the dropdown with icons
const populateDropdown = (dropdown, icons) => {
    dropdown.innerHTML = '<option value="">Select an Icon</option>';
    if (!Array.isArray(icons) || icons.length === 0) {
        console.warn('No icons to populate');
        dropdown.innerHTML = '<option value="">No icons available</option>';
        return;
    }
    icons.forEach(icon => {
        const option = document.createElement('option');
        option.value = `/images/${icon}`;
        option.textContent = icon.replace(/\.(jpg|png)$/i, ''); // Clean file names
        dropdown.appendChild(option);
    });
};

// Fetch icons on page load
fetchIconsWithRetry()
    .then(data => {
        if (data.length === 0) {
            console.warn("No icons found. Displaying default.");
            sharedElements.iconDropdown.innerHTML = '<option value="">No icons available</option>';
        } else {
            populateDropdown(sharedElements.iconDropdown, data);
        }
    })
    .catch(error => {
        console.error("Error fetching icons:", error);
        sharedElements.iconDropdown.innerHTML = '<option value="">Error loading icons</option>';
    });

// Set selected icon
const setSelectedIcon = (iconName) => {
    if (iconName) {
        sharedElements.iconDropdown.value = iconName; // Set dropdown value
        sharedElements.iconPreview.src = iconName; // Update preview image
    } else {
        sharedElements.iconPreview.src = '/images/Button_Default_Image.jpg'; // Default icon
    }

    // Handle broken image links
    sharedElements.iconPreview.onerror = () => {
        sharedElements.iconPreview.src = '/images/Button_Default_Image.jpg';
    };
};

async function fetchAndPopulateNetworks() {
    try {
        console.log('fetchAndPopulateNetworks called')
        const response = await fetch('/WiFiScan');
        console.log('Fetch Response:', response);

        const networks = await response.json();
        console.log('Networks Retrieved:', networks);

        if (!networks || networks.length === 0) {
            console.log('No networks found.');
            sharedElements.ssidDropdown.innerHTML = '<option value="">No networks found</option>';
            return;
        }

        sharedElements.ssidDropdown.innerHTML = '<option value="">Select a Network</option>';
        networks.forEach(network => {
            console.log(`Adding network: SSID=${network.ssid}, Signal=${network.signal}`);
            const option = document.createElement('option');
            option.value = network.ssid;
            option.textContent = `${network.ssid} (${network.signal} dBm)`;
            sharedElements.ssidDropdown.appendChild(option);
        });
        console.log('Dropdown populated successfully.');
    } catch (error) {
        console.error('Error during Wi-Fi scan:', error);
        sharedElements.ssidDropdown.innerHTML = '<option value="">Error loading networks</option>';
    }
}


// Display System Time Function
async function displaySystemTime() {
    try {
        const response = await fetchFromAPI('/GetSystemTime');
        const { time, utcOffset } = response;
        sharedElements.timeDisplay.textContent = `System Time: ${time} UTC Offset: ${utcOffset}`;
    } catch (error) {
        sharedElements.timeDisplay.textContent = 'Error fetching system time.';
        console.error('Error fetching system time:', error);
    }
}

// Toggle Password Visibility
function togglePasswordVisibility() {
    const passwordInput = document.getElementById('WiFi_password');
    const toggleIcon = document.getElementById('toggle-password');

    if (!passwordInput || !toggleIcon) return;

    toggleIcon.addEventListener('click', () => {
        const isPasswordVisible = passwordInput.type === 'text';
        passwordInput.type = isPasswordVisible ? 'password' : 'text';
        toggleIcon.classList.toggle('fa-eye', isPasswordVisible);
        toggleIcon.classList.toggle('fa-eye-slash', !isPasswordVisible);
    });
}




// Update Header Dynamically Based on Mode
async function updateHeaderMode() {
    try {
        // Fetch mode and SSID from the backend
        const { mode, ssid } = await fetchFromAPI('/CurrentMode');
        console.log('Fetched Mode:', mode, 'SSID:', ssid); // Debugging log

        // Determine if the mode is AP or STA
        const isAPMode = mode === 'AP';
        const isSTAMode = mode === 'STA';

        // Update the header dynamically with the correct mode
        sharedElements.headerTitle.textContent = `Settings (${isAPMode ? 'Access Point Mode' : 'Station Mode'})`;

        // Update the connection status dynamically next to the Wi-Fi SSID
        sharedElements.connectionStatus.textContent = isAPMode
            ? '(Connected to Casper_Automation)' // AP mode
            : `(${ssid || 'Not connected'})`; // STA mode or no connection

        // Update body data attribute for mode-specific features (optional)
        document.body.setAttribute('data-mode', mode.toLowerCase());

        console.log('Header and mode updated successfully.');
    } catch (error) {
        console.error('Error fetching mode information:', error);
        sharedElements.headerTitle.textContent = 'Settings (Unknown Mode)';
        sharedElements.connectionStatus.textContent = '(Error fetching mode)';
    }
}




// Save AP Credentials
async function saveAPCredentials() {
    const ssid = sharedElements.ssidDropdown.value;
    const WiFiPassword = sharedElements.WiFiPassword.value;
    const alternativeIP = sharedElements.alternativeIpField.value;

    if (!ssid) {
        alert('Please select a Wi-Fi network.');
        return;
    }

    if (alternativeIP && !isValidIP(alternativeIP)) {
        alert('Please enter a valid IP address.');
        return;
    }

    try {
        sharedElements.saveCredentialsButton.disabled = true;
        sharedElements.saveCredentialsButton.textContent = 'Connecting...';

        const body = JSON.stringify({
            ssid,
            WiFiPassword,
            alternativeIP
        });

        const response = await fetch('/SaveAPConfig', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
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
        sharedElements.saveCredentialsButton.disabled = false;
        sharedElements.saveCredentialsButton.textContent = 'Save Credentials and Connect to Wi-Fi Network';
    }
}

// Initialize Shared Features
function initializeSharedFeatures() {
    fetchAndPopulateNetworks();
    fetchIconsWithRetry();
    displaySystemTime();
    togglePasswordVisibility();
	toggleWiFiPasswordVisibility()
}

// Initialize AP Mode
function initializeAPMode() {
    console.log('Initializing AP Mode...');
    sharedElements.saveCredentialsButton?.addEventListener('click', saveAPCredentials);
    sharedElements.factoryResetButton?.addEventListener('click', async () => {
        try {
            await fetch('/FactoryReset', { method: 'POST' });
            alert('Factory Reset Successful. Rebooting...');
            location.reload();
        } catch (error) {
            console.error('Error performing factory reset:', error);
            alert('Failed to perform factory reset.');
        }
    });
    initializeSharedFeatures();
}

// Initialize STA Mode
function initializeSTAMode() {
    console.log('Initializing STA Mode...');
    sharedElements.switchSelect?.addEventListener('change', async () => {
        const switchNumber = sharedElements.switchSelect.value;
        try {
            const settings = await fetchFromAPI(`/load-switch-settings?s=${switchNumber}`);
            document.getElementById('switch-name').value = settings.switchName;
            sharedElements.iconDropdown.value = settings.iconName;
        } catch (error) {
            console.error('Error loading switch settings:', error);
            alert('Failed to load switch settings.');
        }
    });
    sharedElements.saveSettingsButton?.addEventListener('click', save-sta-settings);
    initializeSharedFeatures();
}

// Save Settings Button Logic
document.getElementById('save-sta-settings').addEventListener('click', async () => {
    // Gather switch configuration
    const switchNumber = document.getElementById('switch-select')?.value || null;
    const switchName = document.getElementById('switch-name')?.value.trim() || null;
    const iconName = document.getElementById('icon-dropdown')?.value || null;

    // Gather user credentials
    const username = document.getElementById('username')?.value.trim() || null;
    const password = document.getElementById('password')?.value.trim() || null;

    // Gather timer configurations
    const timers = [];
    for (let i = 1; i <= 5; i++) {
        timers.push({
            on: document.getElementById(`timer${i}-on`)?.value || "00:00",
            off: document.getElementById(`timer${i}-off`)?.value || "00:00"
        });
    }

    // Validate required inputs for switches
    if (!switchName || !iconName) {
        alert("Please provide a switch name and select an icon.");
        return;
    }

    // Build payload for switch settings and user credentials
    const payload = {
        switchNumber: switchNumber ? parseInt(switchNumber, 10) : null,
        switchName,
        iconName,
        username,
        password,
        timers
    };

    try {
        // Send the payload to the backend
        const response = await fetch('/SaveSettings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        const result = await response.json();

        // Notify the user of the result
        alert(result.status === "success" ? "Settings saved successfully!" : "Failed to save settings.");
    } catch (error) {
        console.error("Error saving settings:", error);
        alert("An error occurred while saving settings. Please try again.");
    }
});


document.addEventListener('DOMContentLoaded', async () => {
    await updateHeaderMode(); // Update mode first
    console.log('Header mode updated successfully');

    const isAPMode = document.body.dataset.mode === 'ap';
    if (isAPMode) {
        initializeAPMode();
    } else {
        initializeSTAMode();
    }

    console.log('Fetching available networks on page load...');
    fetchAndPopulateNetworks(); // Always fetch networks on page load

    console.log('Fetching available icons on page load...');
    fetchIconsWithRetry()
        .then(data => {
            if (data.length === 0) {
                console.warn("No icons found. Displaying default.");
                sharedElements.iconDropdown.innerHTML = '<option value="">No icons available</option>';
            } else {
                populateDropdown(sharedElements.iconDropdown, data);
            }
        })
        .catch(error => {
            console.error("Error fetching icons:", error);
            sharedElements.iconDropdown.innerHTML = '<option value="">Error loading icons</option>';
        });
	toggleWiFiPasswordVisibility(); // Ensure password toggle is initialized
    initializeSharedFeatures(); // Initialize all shared features
    toggleWiFiPasswordVisibility(); // Ensure password toggle is initialized
});



sharedElements.ssidDropdown.addEventListener('focus', () => {
    console.log('Dropdown focused. Fetching networks...');
    fetchAndPopulateNetworks();
});

sharedElements.ssidDropdown.addEventListener('click', () => {
    console.log('Dropdown clicked. Fetching networks...');
    fetchAndPopulateNetworks();
});
