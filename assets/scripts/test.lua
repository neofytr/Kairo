-- test script for Kairo engine
kairo.log("hello from Lua!")
kairo.log("entity count: " .. kairo.entity_count())

function on_update(entity_id, dt)
    -- called each frame for scripted entities
    -- can be used for custom gameplay logic
end

function on_init()
    kairo.log("script initialized")
end

on_init()
