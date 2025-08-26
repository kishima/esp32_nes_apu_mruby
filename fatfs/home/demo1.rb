require "nes-apu"

class ApuWrapper
  def initialize(apu)
    @apu = apu
  end

  def set(channel, setting, value)
    case channel
    when :pulse1
      set_pulse1(setting, value)
    when :pulse2
      set_pulse2(setting, value)
    when :triangle
      set_triangle(setting, value)
    when :noise
      set_noise(setting, value)
    when :dmc
      set_dmc(setting, value)
    when :control
      set_control(setting, value)
    else
      puts "Unknown channel: #{channel}"
    end
  end

  def set_pulse1(setting, value)
    case setting
    when :volume
      @apu.write_reg(0x4000, value & 0x0F)
    when :constant
      reg = @apu.read_reg(0x4000) || 0
      @apu.write_reg(0x4000, (reg & 0xEF) | ((value & 1) << 4))
    when :loop
      reg = @apu.read_reg(0x4000) || 0
      @apu.write_reg(0x4000, (reg & 0xDF) | ((value & 1) << 5))
    when :duty
      reg = @apu.read_reg(0x4000) || 0
      @apu.write_reg(0x4000, (reg & 0x3F) | ((value & 3) << 6))
    when :sweep
      @apu.write_reg(0x4001, value)
    when :timer_low
      @apu.write_reg(0x4002, value & 0xFF)
    when :timer_high
      reg = @apu.read_reg(0x4003) || 0
      @apu.write_reg(0x4003, (reg & 0xF8) | (value & 0x07))
    when :length
      reg = @apu.read_reg(0x4003) || 0
      @apu.write_reg(0x4003, (reg & 0x07) | ((value & 0x1F) << 3))
    else
      puts "Unknown pulse1 setting: #{setting}"
    end
  end

  def set_pulse2(setting, value)
    case setting
    when :volume
      @apu.write_reg(0x4004, value & 0x0F)
    when :constant
      reg = @apu.read_reg(0x4004) || 0
      @apu.write_reg(0x4004, (reg & 0xEF) | ((value & 1) << 4))
    when :loop
      reg = @apu.read_reg(0x4004) || 0
      @apu.write_reg(0x4004, (reg & 0xDF) | ((value & 1) << 5))
    when :duty
      reg = @apu.read_reg(0x4004) || 0
      @apu.write_reg(0x4004, (reg & 0x3F) | ((value & 3) << 6))
    when :sweep
      @apu.write_reg(0x4005, value)
    when :timer_low
      @apu.write_reg(0x4006, value & 0xFF)
    when :timer_high
      reg = @apu.read_reg(0x4007) || 0
      @apu.write_reg(0x4007, (reg & 0xF8) | (value & 0x07))
    when :length
      reg = @apu.read_reg(0x4007) || 0
      @apu.write_reg(0x4007, (reg & 0x07) | ((value & 0x1F) << 3))
    else
      puts "Unknown pulse2 setting: #{setting}"
    end
  end

  def set_triangle(setting, value)
    case setting
    when :control
      @apu.write_reg(0x4008, value)
    when :timer_low
      @apu.write_reg(0x400A, value & 0xFF)
    when :timer_high
      reg = @apu.read_reg(0x400B) || 0
      @apu.write_reg(0x400B, (reg & 0xF8) | (value & 0x07))
    when :length
      reg = @apu.read_reg(0x400B) || 0
      @apu.write_reg(0x400B, (reg & 0x07) | ((value & 0x1F) << 3))
    else
      puts "Unknown triangle setting: #{setting}"
    end
  end

  def set_noise(setting, value)
    case setting
    when :volume
      @apu.write_reg(0x400C, value & 0x0F)
    when :constant
      reg = @apu.read_reg(0x400C) || 0
      @apu.write_reg(0x400C, (reg & 0xEF) | ((value & 1) << 4))
    when :loop
      reg = @apu.read_reg(0x400C) || 0
      @apu.write_reg(0x400C, (reg & 0xDF) | ((value & 1) << 5))
    when :period
      @apu.write_reg(0x400E, value & 0x0F)
    when :loop_noise
      reg = @apu.read_reg(0x400E) || 0
      @apu.write_reg(0x400E, (reg & 0x7F) | ((value & 1) << 7))
    when :length
      @apu.write_reg(0x400F, (value & 0x1F) << 3)
    else
      puts "Unknown noise setting: #{setting}"
    end
  end

  def set_dmc(setting, value)
    case setting
    when :rate
      @apu.write_reg(0x4010, value & 0x0F)
    when :loop
      reg = @apu.read_reg(0x4010) || 0
      @apu.write_reg(0x4010, (reg & 0xBF) | ((value & 1) << 6))
    when :irq
      reg = @apu.read_reg(0x4010) || 0
      @apu.write_reg(0x4010, (reg & 0x7F) | ((value & 1) << 7))
    when :direct
      @apu.write_reg(0x4011, value & 0x7F)
    when :address
      @apu.write_reg(0x4012, value)
    when :length
      @apu.write_reg(0x4013, value)
    else
      puts "Unknown dmc setting: #{setting}"
    end
  end

  def set_control(setting, value)
    case setting
    when :enable
      @apu.write_reg(0x4015, value & 0x1F)
    when :frame_counter
      @apu.write_reg(0x4017, value)
    else
      puts "Unknown control setting: #{setting}"
    end
  end
end

def show_info
  puts "=== NES APU Registers ==="
  puts ""
  puts "-- Pulse 1 ($4000-$4003) --"
  puts "$4000: DDLC VVVV - Duty, Loop, Constant, Volume"
  puts "$4001: EPPP NSSS - Sweep Enable, Period, Negate, Shift"
  puts "$4002: TTTT TTTT - Timer low 8 bits"
  puts "$4003: LLLL LTTT - Length counter, Timer high 3 bits"
  puts ""
  puts "-- Pulse 2 ($4004-$4007) --"
  puts "$4004: DDLC VVVV - Duty, Loop, Constant, Volume"
  puts "$4005: EPPP NSSS - Sweep Enable, Period, Negate, Shift"
  puts "$4006: TTTT TTTT - Timer low 8 bits"
  puts "$4007: LLLL LTTT - Length counter, Timer high 3 bits"
  puts ""
  puts "-- Triangle ($4008-$400B) --"
  puts "$4008: CRRR RRRR - Control, Reload value"
  puts "$4009: ---- ---- - Unused"
  puts "$400A: TTTT TTTT - Timer low 8 bits"
  puts "$400B: LLLL LTTT - Length counter, Timer high 3 bits"
  puts ""
  puts "-- Noise ($400C-$400F) --"
  puts "$400C: --LC VVVV - Loop, Constant, Volume"
  puts "$400D: ---- ---- - Unused"
  puts "$400E: L--- PPPP - Loop noise, Period"
  puts "$400F: LLLL L--- - Length counter"
  puts ""
  puts "-- DMC ($4010-$4013) --"
  puts "$4010: IL-- RRRR - IRQ, Loop, Rate"
  puts "$4011: -DDD DDDD - Direct load"
  puts "$4012: AAAA AAAA - Sample address"
  puts "$4013: LLLL LLLL - Sample length"
  puts ""
  puts "-- Control/Status --"
  puts "$4015: ---D NT21 - Enable DMC, Noise, Triangle, Pulse2, Pulse1"
  puts "$4017: FI-- ---- - Frame counter (5/4 step, IRQ disable)"
  puts "===================="
end

apu = NesApu.new()
wrapper = ApuWrapper.new(apu)

show_info

# Usage: wrapper.set(:channel, :setting, value)
# Supported channels: :pulse1, :pulse2, :triangle, :noise, :dmc, :control
# Example settings per channel:
# - Pulse channels: :volume, :duty, :constant, :loop, :sweep, :timer_low, :timer_high, :length
# - Triangle: :control, :timer_low, :timer_high, :length
# - Noise: :volume, :constant, :loop, :period, :loop_noise, :length
# - DMC: :rate, :loop, :irq, :direct, :address, :length
# - Control: :enable, :frame_counter

puts "\n=== Example Usage ==="
puts "Enable all channels:"
wrapper.set(:control, :enable, 0x1F)

puts "Set Pulse1 to play middle C (262Hz):"
loop do
  wrapper.set(:pulse1, :duty, 2)
  wrapper.set(:pulse1, :volume, 15)
  wrapper.set(:pulse1, :constant, 1)
  wrapper.set(:pulse1, :timer_low, 0xFD)
  wrapper.set(:pulse1, :timer_high, 0x02)
  wrapper.set(:pulse1, :length, 0x10)

  apu.process(1)
  Machine.vtaskdelay(16)
end
