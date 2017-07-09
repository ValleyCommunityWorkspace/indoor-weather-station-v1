<?php

# https://server.org/updateweatherstation.php?devid=12345&devname=FOO&issueid=Lounge&indoortemp=0&indoorhumidity=0
date_default_timezone_set('Pacific/Auckland');

function get_ip() {
		//Just get the headers if we can or else use the SERVER global
		if ( function_exists( 'apache_request_headers' ) ) {
			$headers = apache_request_headers();
		} else {
			$headers = $_SERVER;
		}
		//Get the forwarded IP if it exists
		if ( array_key_exists( 'X-Forwarded-For', $headers ) && filter_var( $headers['X-Forwarded-For'], FILTER_VALIDATE_IP, FILTER_FLAG_IPV4 ) ) {
			$the_ip = $headers['X-Forwarded-For'];
		} elseif ( array_key_exists( 'HTTP_X_FORWARDED_FOR', $headers ) && filter_var( $headers['HTTP_X_FORWARDED_FOR'], FILTER_VALIDATE_IP, FILTER_FLAG_IPV4 )
		) {
			$the_ip = $headers['HTTP_X_FORWARDED_FOR'];
		} else {
			
			$the_ip = filter_var( $_SERVER['REMOTE_ADDR'], FILTER_VALIDATE_IP, FILTER_FLAG_IPV4 );
		}
		return $the_ip;
	}


header("Content-type: text/plain");
header("Cache-Control: no-cache, must-revalidate"); // HTTP/1.1
header("Expires: Sat, 26 Jul 1997 01:00:00 GMT");

$ip = get_ip();

$ua = preg_split("/\r?\n/",$_SERVER['HTTP_USER_AGENT'],1)[0];
str_replace(",", "",$ua);

//$date = $_GET["dateutc"];

$nowGMTepoch = time();  # number of seconds since the Unix Epoch (January 1 1970 00:00:00 GMT)

$uid = intval( $_GET["devid"],16);
$uidString = strtoupper(dechex($uid));
if ($uid == 0 ) { header($_SERVER['SERVER_PROTOCOL'] . ' 400 Bad Request', true, 400); die("Not found"); }

preg_match('/\b(\w+)\b/', $_GET["devname"], $matches);
$devname = $matches[0];
str_replace(",", "",$devname);

#$issueid = intval( $_GET["issueid"],16);
#if ($issueid == 0 ) { header($_SERVER['SERVER_PROTOCOL'] . ' 400 Bad Request', true, 400); die("Not found"); }
#$issueidString = strtoupper(dechex($issueid));

preg_match('/\b(\w+)\b/', $_GET["issueid"], $matches);
$issueidString = $matches[0];
str_replace(",", "",$issueidString);

$indoortemp = floatval($_GET["indoortemp"]); 
$indoorhumidity = floatval($_GET["indoorhumidity"]);


# "ip","ua","id","devname","issueid","time_t","indoortemp","indoorhumidity"
$inserts = array(
                  array('ip' => $ip,
                        'ua' => $ua,
                        'id' => $uidString,
			'devname' => $devname,
			'issueid' => $issueidString,
			'time_t' => $nowGMTepoch,
			'indoortemp' => $indoortemp,
			'indoorhumidity' => $indoorhumidity )
                );

    // Create (connect to) SQLite database in file
    $file_db = new PDO('sqlite:/var/log/indoorwx/rawdata.sqlite');
    // Set errormode to exceptions
    $file_db->setAttribute(PDO::ATTR_ERRMODE, 
                            PDO::ERRMODE_EXCEPTION); 


    // Prepare INSERT statement to SQLite3 file db
    $insert = "INSERT INTO rawdata (ip,ua,id,devname,issueid,time_t,indoortemp,indoorhumidity) 
                VALUES (:ip,:ua,:id,:devname,:issueid,:time_t,:indoortemp,:indoorhumidity)";
    $stmt = $file_db->prepare($insert);
 
    // Bind parameters to statement variables
    $stmt->bindParam(':ip', $_ip);
    $stmt->bindParam(':ua', $_ua);
    $stmt->bindParam(':id', $_id);
    $stmt->bindParam(':devname', $_devname);
    $stmt->bindParam(':issueid', $_issueid);
    $stmt->bindParam(':time_t', $_time_t);
    $stmt->bindParam(':indoortemp', $_indoortemp);
    $stmt->bindParam(':indoorhumidity', $_indoorhumidity);
 
    // Loop thru all messages and execute prepared insert statement
    foreach ($inserts as $m) {
      // Set values to bound variables
      $_ip = $m['ip'];
      $_ua = $m['ua'];
      $_id = $m['id'];
      $_devname = $m['devname'];
      $_issueid = $m['issueid'];
      $_time_t = $m['time_t'];
      $_indoortemp = $m['indoortemp'];
      $_indoorhumidity = $m['indoorhumidity'];
 
      // Execute statement
      $stmt->execute();
    }
 


    // Close file db connection
    $file_db = null;

echo 'GMT:'. $nowGMTepoch ."\n";
