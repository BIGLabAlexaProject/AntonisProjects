# AntonisProjects
AWS IoT, Lambda and IAM policies for setting up rPi 
* Setup IAM Role access settings
    * https://console.aws.amazon.com/iam/home > Roles > Create Role > 
      * Select AWS Service and IoT > Next: Permissions :
      * AWSIoTLogging:            Allows creation of Amazon CloudWatch Log groups and streaming logs to the groups
      * AWSIoTRuleActions:        Allows access to all AWS services supported in AWS IoT Rule Actions
      * AWSIoTThingsRegistration: This policy allows users to register things at bulk using AWS IoT StartThingRegistrationTask 
* Setup Cross Account Access
  * https://console.aws.amazon.com/iam/home > Roles > Create Role > 
  * Select Another AWS account > Add ID number , uncheck boxes 
  * Add Policies accordingl ... ( improve) 
*  Setup AWS IoT:
   *  https://console.aws.amazon.com/iot/home
   *  Instructions Here: https://docs.aws.amazon.com/iot/latest/developerguide/iot-sdk-setup.html
   *  Set region to EU West (Ireland) for Europe 
   *  Setup a policy: 
      *  Secure> Plocies > Create a Policy > Advance Mode 
      *  Copy the contents from aws_iot_policy.txt to the policy statement window. 
      *  Add a name i.e. rPiPolicy 
   *  Create a thing:  
      *  Manage> Register a Thing > Create a single thing
      *  Just Providing a name is enough> Next >One-click certificate creation (recommended)
      *  Download Certificates: 
         *  xxxxxxxxxx-certificate.pem.crt
         *  xxxxxxxxxx-public.pem.key
         *  xxxxxxxxxx-private.pem.key
         *  ca.crt
      *  Attach rpiPolicy 
      *  Secure > Certificates > Activate 
   *  Manage> Things > Select Thing 
      *  Copy arn arn:aws:iot:eu-west-1:xxxxxxxxxxxx:thing/xxxxxxxxxxxx                                        
*  Prepare rPi
   *  Download raspbian wirte image on sd card 
   *  Open fat partition and make a file > ssh (with no extension) 
   *  https://hackernoon.com/raspberry-pi-headless-install-462ccabd75d0
   *  Use a DHCP router for 1st time to connect to ssh. 
* ssh connection    
      *  ssh pi@192.168.2.2 -X
*  



   * 
      
         

Setup RPI as IoT 

1. Add to AWS IoT console 
  https://eu-west-1.console.aws.amazon.com/iot/home?region=eu-west-1#/thing/MyRaspberryPi
2. Setup Folder with Certificates /av1869/Alexa Project 
3. Use AWS IoT instructions pdf annotation (iot-dg.pdf)
4. Setup Dependencies : 
5.  cd aws-iot-device-sdk-embedded-C/external_libs
6. git clone https://github.com/ARMmbed/mbedtls
7. change folder name to mbedTLS delete other folder 

http://docs.yottabuild.org/#installing
Yotta is broken --  pip install pip==9.0.3

openssl s_client -connect a2ze46vjacw0c0.iot.eu-west-1.amazonaws.com:8443 -CAfile CA.pem -cert fe0b7d79c5-certificate.pem.crt -key fe0b7d79c5-private.pem.key
