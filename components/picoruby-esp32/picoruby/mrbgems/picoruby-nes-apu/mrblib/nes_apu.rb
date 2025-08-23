class NesApu
  def initialize()
    _init()
  end

  def reset()
    # Reset all APU registers to default state
    # Clear all channels
    (0x4000..0x4013).each do |addr|
      write_reg(addr, 0)
    end
    # Disable frame counter interrupt
    write_reg(0x4017, 0x40)
    # Enable all channels
    write_reg(0x4015, 0x0F)
  end
end
