require 'csv'
require 'serialport'

port = SerialPort.new('/dev/cu.usbserial-DJ00DN7Z', 115200)

interval = 0.1
ti = ((1.0/(interval * 2))*1*60).to_i
timestamp = []
puts(ti)
sleep(1)
ti.times do
    timestamp << [Time.now().to_f, 1]
    port.write('1')
    sleep(interval)
    port.write('0')
    sleep(interval)
end


CSV.open("Hardware_timestamp.csv", "wb") do |csv|

    csv << ["timestamp", "target"]
    timestamp.each do |t|
        csv << t
    end
end

puts('Done Tag')
