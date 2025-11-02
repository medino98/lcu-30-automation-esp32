// Initialize Vanta.js background
VANTA.NET({
    el: "#vanta-bg",
    mouseControls: true,
    touchControls: true,
    gyroControls: false,
    minHeight: 200.00,
    minWidth: 200.00,
    scale: 1.00,
    scaleMobile: 1.00,
    color: 0x4f46e5,
    backgroundColor: 0xf9fafb,
    points: 12.00,
    maxDistance: 22.00,
    spacing: 18.00
});

// Initialize Feather Icons
feather.replace();

// Device Management Logic
document.addEventListener('DOMContentLoaded', function() {
    const modal = document.getElementById('addDeviceModal');
    const addDeviceBtn = document.getElementById('addDeviceBtn');
    const closeModalBtn = document.getElementById('closeModalBtn');
    const cancelBtn = document.getElementById('cancelBtn');
    const deviceForm = document.getElementById('deviceForm');
    const devicesList = document.getElementById('devicesList');
    const deviceCount = document.getElementById('deviceCount');
    const accessLogs = document.getElementById('accessLogs');
    
    let deviceCounter = 3; // Starting with 3 sample devices

    // Show modal
    addDeviceBtn.addEventListener('click', function() {
        modal.classList.remove('invisible', 'opacity-0');
        modal.querySelector('div').classList.remove('scale-95');
        modal.querySelector('div').classList.add('scale-100');
    });

    // Hide modal
    function hideModal() {
        modal.classList.add('invisible', 'opacity-0');
        modal.querySelector('div').classList.remove('scale-100');
        modal.querySelector('div').classList.add('scale-95');
        deviceForm.reset();
    }

    closeModalBtn.addEventListener('click', hideModal);
    cancelBtn.addEventListener('click', hideModal);

    // Handle form submission
    deviceForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const deviceName = document.getElementById('deviceName').value;
        const macAddress = document.getElementById('macAddress').value;
        const accessLevel = document.querySelector('input[name="accessLevel"]:checked').value;
        
        // Create new device card
        const newDevice = document.createElement('div');
        newDevice.className = 'device-card bg-white rounded-lg border border-gray-200 p-4 transition-all duration-300';
        newDevice.innerHTML = `
            <div class="flex items-start">
                <div class="p-3 rounded-full bg-indigo-100 mr-4">
                    <i data-feather="smartphone" class="text-indigo-600 w-5 h-5"></i>
                </div>
                <div class="flex-1">
                    <h3 class="font-medium text-gray-800">${deviceName}</h3>
                    <p class="text-sm text-gray-500 mt-1">MAC: ${macAddress}</p>
                    <div class="flex items-center mt-3">
                        <span class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-yellow-100 text-yellow-800">
                            Pending
                        </span>
                        <span class="ml-2 text-xs text-gray-500">Added just now</span>
                    </div>
                </div>
                <button class="text-gray-400 hover:text-gray-600">
                    <i data-feather="more-vertical" class="w-5 h-5"></i>
                </button>
            </div>
        `;
        
        // Add to the beginning of the devices list
        devicesList.insertBefore(newDevice, devicesList.firstChild);
        
        // Create access log entry
        const newLog = document.createElement('div');
        newLog.className = 'px-6 py-4 flex items-center';
        newLog.innerHTML = `
            <div class="p-2 rounded-full bg-blue-100 mr-4">
                <i data-feather="user-plus" class="text-blue-600 w-4 h-4"></i>
            </div>
            <div class="flex-1">
                <p class="text-sm font-medium text-gray-800">New device added: ${deviceName}</p>
                <p class="text-xs text-gray-500 mt-1">MAC: ${macAddress} • just now</p>
            </div>
        `;
        
        // Add to the beginning of the access logs
        accessLogs.insertBefore(newLog, accessLogs.firstChild);
        
        // Update device count
        deviceCounter++;
        deviceCount.textContent = `${deviceCounter} Active`;
        
        // Hide modal and reset form
        hideModal();
        
        // Refresh icons
        feather.replace();
    });

    // Close modal when clicking outside
    modal.addEventListener('click', function(e) {
        if (e.target === modal) {
            hideModal();
        }
    });
});