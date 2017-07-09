suppressPackageStartupMessages({
  library(dygraphs)
  require(lubridate)
  require(dplyr)
  require(xts)
})

nxts <- function(x,order.by,name) {
  x <- xts(x,order.by)
  names(x) <- name
  x
}

resample.approx.xts <- function(x,m, maxgap=5) {
  # resample an xts every m minutes
  f <- round_date(min(index(x)),unit=paste(m,"mins"))
  t <- round_date(max(index(x)),unit=paste(m,"mins"))
  
  s <- seq.POSIXt(from=f,to=t,by=paste(m,"mins"))
  
  newxts <- merge(x,xts(,s))
  if (all(is.na(newxts))) {
    a <- newxts
  } else {
    a <- na.approx(newxts,maxgap=maxgap, na.rm=FALSE)
  }
  
  a[s]  # return xts at each resample point
}

Sys.setenv(TZ = "Pacific/Auckland")

oneweekago <- as.numeric(Sys.time() - weeks(1))

dbfile <- "/var/log/indoorwx/rawdata.sqlite"  # rawdata (ip,ua,id,devname,issueid,time_t,indoortemp,indoorhumidity) 

rawdat_sqlite <- tbl(src_sqlite(dbfile), "rawdata")
ids = c("DCB_RECEPTION1","DCB_RECEPTION2","DCB_COMMUNAL","DCB_THEATRE")
system.time({
  dat <- rawdat_sqlite %>% select(devname,issueid,time_t,indoortemp,indoorhumidity) %>% 
    filter(issueid %in% ids) %>% 
     filter(time_t > oneweekago) %>%
    arrange(issueid,time_t) %>%
    collect()
})

dat$date <- as.POSIXct(dat$time_t, tz = "Pacific/Auckland", origin = "1970-01-01") 

dat$indoortemp[dat$indoortemp==0] <- NA

xtsdf <- dat %>% group_by(issueid) %>% arrange(date) %>% do(
  temp=nxts(.$indoortemp,.$date,make.names(.$issueid[[1]])) %>% resample.approx.xts(5),
  humid=nxts(.$indoorhumidity,.$date,make.names(.$issueid[[1]])) %>% resample.approx.xts(5)
)


dygraph(do.call(merge,xtsdf$temp)) %>% dyRangeSelector() %>% dyAxis("y",valueRange=c(10,25))
dygraph(do.call(merge,xtsdf$humid)) %>% dyRangeSelector() %>% dyAxis("y",valueRange=c(0,80))

dygraph(merge(do.call(merge,xtsdf$temp),do.call(merge,xtsdf$humid))) %>% dyRangeSelector() %>% dyAxis("y",valueRange=c(0,80))





