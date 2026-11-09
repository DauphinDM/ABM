// ABM dynamic data never reaches Omarchy's shared rich-text tooltip sink.
function parse(text, now) {
    if (typeof text !== "string" || text.length > 65536) return {}
    try {
        var data = JSON.parse(text)
        if (!data || typeof data !== "object" || data.connected !== true) return {}
        var age = now - data.updatedAt
        if (typeof data.updatedAt !== "number" || !isFinite(age) || age < -1000 || age > 3000) return {}
        var t = data.telemetry || {}
        return {
            connected: true,
            multiplayer: data.multiplayer === true,
            speed: typeof t.speed === "number" && isFinite(t.speed) && t.speed >= 0 && t.speed <= 2000 ? t.speed : null,
            playerCount: typeof data.playerCount === "number" && data.playerCount >= 0 && data.playerCount <= 10000 ? Math.floor(data.playerCount) : null,
            rpm: numeric(t.rpm, 0, 100000), fps: numeric(data.fps, 0, 2000),
            fuel: numeric(t.fuel, 0, 1), ping: numeric(data.ping, 0, 60000),
            server: cleanTitle(data.server), vehicle: cleanTitle(data.vehicleName || data.vehicle)
        }
    } catch (e) { return {} }
}
function numeric(value, low, high) { return typeof value === "number" && isFinite(value) && value >= low && value <= high ? value : null }
function cleanTitle(value) { return typeof value === "string" ? value.slice(0, 512).replace(/\^[0-9A-Za-z*]/g, "").replace(/[\x00-\x1f\u202a-\u202e\u2066-\u2069]/g, " ") : "" }
function clickAction(button, gameRunning) { return button === 1 ? (gameRunning === false ? "start" : "toggle") : button === 2 || button === 4 ? "launch" : "none" }
