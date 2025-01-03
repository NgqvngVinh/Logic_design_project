function updateSensorData() {
    fetch('/info')
        .then(response => response.json())
        .then(data => {
            document.documentElement.style.setProperty('--temp', data.temperature);
            document.querySelector(
                '.dashboard .gauge .circle[data-type="temperature"] .value'
            ).textContent = data.temperature + "<br>" + "°C";
            
            document.documentElement.style.setProperty('--humi', data.humidity);
            document.querySelector(
                '.dashboard .gauge .circle[data-type="humidity"] .value'
            ).textContent = data.humidity + "<br>" + "%";

            document.documentElement.style.setProperty('--mois', data.moisture);
            document.querySelector(
                '.dashboard .gauge .circle[data-type="moisture"] .value'
            ).textContent = data.moisture + "<br>" + "Value";

            document.documentElement.style.setProperty('--ligh', data.light);
            document.querySelector(
                '.dashboard .gauge .circle[data-type="light"] .value'
            ).textContent = data.light + "<br>" + "Lux";
        })
        .catch(error => console.error('Error:', error));
}

setInterval(updateSensorData, 1000);