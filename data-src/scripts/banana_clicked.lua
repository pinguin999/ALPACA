print("banana_clicked")

function respawn()
    print("respawn")
    PlayAnimationOn("Player", 0, "idle", true, pass)
end

function play_death()
    PlayAnimationOn("Player", 0, "death", false, respawn)
end

GoToPointOn("banana", "center", play_death)
