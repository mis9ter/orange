-- ZIPVFS metadata checks.
--
-- The first query checks all data blocks and freelist blocks to ensure
-- no two blocks overlap.  If any blocks do overlap, they cause a row of
-- output to appear.  Data block have a non-zero pagenumber and freelist
-- blocks have  page number of 0.
--

SELECT 'OVERLAP', pgno, span, ppgno, pspan FROM (
  WITH blocks(first,last,pgno) AS (
    SELECT offset AS first, offset+size-1 AS last, pgno
      FROM zipvfs
     UNION ALL
    SELECT offset, offset+size-1, 0
      FROM zipvfs_freelist)
  SELECT pgno,
         printf('%d..%d',first,last) AS span,
         lag(pgno) OVER w1 AS ppgno,
         printf('%d..%d',lag(first) OVER w1, lag(last) OVER w1) AS pspan,
         first<=lag(last) OVER w1 AS isoverlap
    FROM blocks
  WINDOW w1 AS (ORDER BY first, last))
 WHERE isoverlap;

-- Verify that all pages are readable.  Print the page number of any
-- page for which the decompression function fails.
--
SELECT 'UNREADABLE', pgno
  FROM sqlite_dbpage
 WHERE data IS NULL;

-- Since we are checking things, we might as well also do a quick_check.
-- It is unclear if the queries above would find anything that quick_check
-- would not, though the queries above do provide additional detail about
-- went wrong, whereas quick_check just says that the ZIPVFS file is malformed.
--
PRAGMA quick_check;
