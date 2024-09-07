print("banana_clicked")

GoToPointOn("banana", "center", function ()
    PlayAnimationOn("Player", 0, "death", false, function ()
        print("respawn")
        PlayAnimationOn("Player", 0, "idle", true)
    end)
end)
