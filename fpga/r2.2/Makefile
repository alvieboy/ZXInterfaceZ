TOP=interfacez

all: output_files/$(TOP).rbf

output_files/$(TOP).rbf: db/$(TOP).map.cdb

db/$(TOP).map.cdb:
	quartus_map --read_settings_files=on --write_settings_files=off $(TOP) -c $(TOP)

db/$(TOP).cmp.cdb: db/$(TOP).map.cdb
	quartus_fit --read_settings_files=on --write_settings_files=off $(TOP) -c $(TOP)

output_files/$(TOP).rbf: db/$(TOP).cmp.cdb
	quartus_asm --read_settings_files=on --write_settings_files=off $(TOP) -c $(TOP)

clean:
	rm -f output_files/$(TOP).rbf  db/$(TOP).map.cdb db/$(TOP).map.cdb

#quartus_sta interfacez -c interfacez
