<?php
unset($onlineip);

if(getenv('HTTP_CLIENT_IP')){
     $onlineip=getenv('HTTP_CLIENT_IP');
}elseif(getenv('HTTP_X_FORWARDED_FOR')){
     $onlineip=getenv('HTTP_X_FORWARDED_FOR');
}else{
     $onlineip=getenv('REMOTE_ADDR');
}

echo $onlineip;
?>