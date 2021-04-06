select *,row_number() over() rownum, ntile(5) over() ntile, min(a) over (partition by b) min, max(a) over (partition by b) max, avg(a) over (partition by b) avg, lag(c,1) over (order by a) lag, lead(c,1) over (order by a) lead, nth_value(a,2) over (partition by b order by a) nth, first_value(a) over (partition by b order by a) first, last_value(a) over (partition by b order by a) last 
from win;
.quit
