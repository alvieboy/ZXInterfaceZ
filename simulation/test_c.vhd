
configuration t001 of tb_top is
    for sim
        for tbc: tbc_device
            use entity work.tbc_device(t001);
        end for;
    end for;
end t001;

configuration t002 of tb_top is
    for sim
        for tbc: tbc_device
            use entity work.tbc_device(t002);
        end for;
    end for;
end t002;

configuration t003 of tb_top is
    for sim
        for tbc: tbc_device
            use entity work.tbc_device(t003);
        end for;
    end for;
end t003;
