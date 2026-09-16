function set_direction(angle)
    if angle > -90 and angle < 90 then
        SetPlayerScaleX(1.0)
    else
        SetPlayerScaleX(-1.0)
    end
end
