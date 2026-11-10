// A model's facets describe any of its configurations, not necessarily one
// configuration that satisfies every selected facet simultaneously.
function values(item, field) {
    var list = item.facets && item.facets[field]
    if (list && list.length) return list
    if (field === "source" && item.custom) return ["Custom"]
    return item[field] ? [item[field]] : []
}
function options(entries, field, label) {
    var found = Object.create(null)
    entries.forEach(function(item) { values(item, field).forEach(function(v) { found[v] = true }) })
    return [{label:label,value:""}].concat(Object.keys(found).sort(function(a,b) { return a.localeCompare(b) })
        .map(function(v) { return {label:v,value:v} }))
}
function sourceMatches(item, filter) {
    var sources = values(item,"source")
    if (filter === "custom") return item.custom === true || sources.indexOf("Custom") >= 0
    if (filter === "stock") return sources.indexOf("BeamNG - Official") >= 0
    if (filter === "mods") return sources.some(function(s) { return ["BeamNG - Official","Custom","Career"].indexOf(s) < 0 })
    return true
}
function select(entries, filters, view, favorites, query, order) {
    query = query.trim().toLowerCase()
    var result = entries.filter(function(item) {
        if (view === "favorites" && favorites.indexOf(item.id) < 0) return false
        if (!sourceMatches(item,view)) return false
        for (var field in filters) {
            if (filters[field] && values(item,field).indexOf(filters[field]) < 0) return false
        }
        var words = [item.id,item.name,item.key,item.description,item.brand]
        for (var key in item.facets || {}) words = words.concat(item.facets[key])
        return words.join(" ").toLowerCase().indexOf(query) >= 0
    })
    result.sort(function(a,b) {
        if (order === "favorites") {
            var first = (favorites.indexOf(b.id) >= 0 ? 1 : 0) - (favorites.indexOf(a.id) >= 0 ? 1 : 0)
            if (first) return first
        }
        var field = ["brand","type","source"].indexOf(order) >= 0 ? order : "name"
        var av = field === "name" ? a.name || a.id : values(a,field).join(" ")
        var bv = field === "name" ? b.name || b.id : values(b,field).join(" ")
        var compared = av.localeCompare(bv) || (a.id || "").localeCompare(b.id || "")
        return order === "reverse" ? -compared : compared
    })
    return result
}
