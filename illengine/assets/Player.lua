local entity = ...
p = GetPosition(entity)
v = GetVelocity(entity)
g = GetGravity(entity)
c = GetCollider(entity)
r = GetHitRate(entity)
r = r + 1
EntitiesWithColliders = GetColliders()
onGround = 0
for i = 1, #EntitiesWithColliders do
    if(not(entity == EntitiesWithColliders[i])) then
        pe = GetPosition(EntitiesWithColliders[i])
        ce = GetCollider(EntitiesWithColliders[i])
        t = IsTrigger(EntitiesWithColliders[i])
        if (not t and Collides(entity, EntitiesWithColliders[i]) and p.y - c.y/2 >= pe.y - ce.y/2) then
            onGround = 1
            break
        else
            onGround = 0
        end
    end
end

if onGround == 1 then
    v.y = 0
else
    v.y = v.y + g
end

if KeyIsDown(KEYBOARD.A) then 
    v.x = -5 
elseif KeyIsDown(KEYBOARD.D) then 
    v.x = 5 
else
    v.x = 0
end

if(onGround == 1 and KeyIsDown(KEYBOARD.SPACE)) then
    v.y = 10
    onGround = 0
end

if(p.y < -100) then
    p.x = -160
    p.y = 0
    v.y = 0
end

p.x = p.x + v.x
p.y = p.y + v.y
SetHitRate(player, r)