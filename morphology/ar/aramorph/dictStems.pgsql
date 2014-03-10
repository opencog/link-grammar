-- Create PostgreSQL table 'stems'
-- To run this type 'dropdb dictStems; createdb dictStems && psql -f dictStems.pgsql dictStems' from the command-line.  You must have appropriate permissions.

CREATE TABLE stems (
  short_ar varchar(20), 
  long_ar varchar(24), 
  pos varchar(22), 
  gloss varchar(90), 
  tagged_ar varchar(70), 
  sense_ar varchar(40)
);

CREATE INDEX short_ar_index ON stems (short_ar);

\copy stems from 'dictStems.tsv'

VACUUM stems;
