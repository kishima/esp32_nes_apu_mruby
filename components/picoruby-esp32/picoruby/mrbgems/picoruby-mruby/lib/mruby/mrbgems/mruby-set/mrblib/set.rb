class Set
  def initialize(enum = nil, &block)
    __init
    return self if enum.nil?

    if block
      __do_with_enum(enum) { add(block.call(_1)) }
    else
      merge(enum)
    end
    self
  end

  # internal method
  def __do_with_enum(enum, &block)
    if enum.respond_to?(:each)
      enum.each(&block)
    else
      raise ArgumentError, "value must be enumerable"
    end
  end

  # Merges the elements of the given enumerable object to the set and returns
  # self.
  #
  # @param [Enumerable] enum The enumerable object to merge elements from
  # @return [Set] self
  def merge(enum)
    __merge(enum) || __do_with_enum(enum) { |o| add(o) }
    self
  end

  # Replaces the contents of the set with the contents of the given enumerable
  # object and returns self.
  #
  # @param [Enumerable] enum The enumerable object to replace with
  # @return [Set] self
  def replace(enum)
    clear
    merge(enum)
  end

  # Deletes every element that appears in the given enumerable object and
  # returns self.
  #
  # @param [Enumerable] enum The enumerable object containing elements to remove
  # @return [Set] self
  def subtract(enum)
    __subtract(enum) || __do_with_enum(enum) { |o| delete(o) }
    self
  end

  # Returns a new set containing elements common to the set and the given
  # enumerable object.
  #
  # @param [Enumerable] enum The enumerable object to find common elements with
  # @return [Set] A new set containing elements common to both
  def intersection(enum)
    __intersection(enum) || begin
      n = Set.new
      __do_with_enum(enum) { |o| n.add(o) if include?(o) }
      n
    end
  end

  # Alias for #intersection
  alias & intersection

  # Returns a new set built by merging the set and the elements of the given
  # enumerable object.
  #
  # @param [Enumerable] enum The enumerable object to merge with
  # @return [Set] A new set containing all elements from both
  def union(enum)
    __union(enum) || dup.merge(enum)
  end

  # Aliases for #union
  alias | union
  alias + union

  # Returns a new set built by duplicating the set, removing every element that
  # appears in the given enumerable object.
  #
  # @param [Enumerable] enum The enumerable object to find elements to remove
  # @return [Set] A new set with elements from self that are not in enum
  def difference(enum)
    __difference(enum) || begin
      result = dup
      __do_with_enum(enum) { |o| result.delete(o) }
      result
    end
  end

  # Alias for #difference
  alias - difference

  # Returns a new set containing elements exclusive between the set and the given
  # enumerable object.
  #
  # @param [Enumerable] enum The enumerable object to find exclusive elements with
  # @return [Set] A new set containing elements exclusive between both
  def ^(enum)
    __xor(enum) || begin
      s2 = Set.new(enum)
      (self | s2) - (self & s2)
    end
  end

  # Iterates over each element in the set.
  #
  # @yield [Object] Each element in the set
  # @return [Set] self
  def each(&block)
    return to_enum :each unless block_given?
    # Use C implementation's to_a method and iterate
    to_a.each(&block)
    self
  end

  # Deletes every element for which the given block returns true.
  #
  # @yield [Object] Each element in the set
  # @yieldreturn [Boolean] true if the element should be deleted
  # @return [Set] self
  def delete_if
    return to_enum :delete_if unless block_given?
    select { yield _1 }.each { delete(_1) }
    self
  end

  # Deletes every element for which the given block returns false.
  #
  # @yield [Object] Each element in the set
  # @yieldreturn [Boolean] true if the element should be kept
  # @return [Set] self
  def keep_if
    return to_enum :keep_if unless block_given?
    reject { yield _1 }.each { delete(_1) }
    self
  end

  # Replaces each element with the result of the given block.
  #
  # @yield [Object] Each element in the set
  # @yieldreturn [Object] The new value for the element
  # @return [Set] self
  def collect!
    return to_enum :collect! unless block_given?
    set = self.class.new
    each { set << yield(_1) }
    replace(set)
  end
  alias map! collect!

  # Deletes every element for which the given block returns true.
  #
  # @yield [Object] Each element in the set
  # @yieldreturn [Boolean] true if the element should be deleted
  # @return [Set] self if any elements were deleted, nil otherwise
  def reject!(&block)
    return to_enum :reject! unless block_given?
    n = size
    delete_if(&block)
    size == n ? nil : self
  end

  # Deletes every element for which the given block returns false.
  #
  # @yield [Object] Each element in the set
  # @yieldreturn [Boolean] true if the element should be kept
  # @return [Set] self if any elements were deleted, nil otherwise
  def select!(&block)
    return to_enum :select! unless block_given?
    n = size
    keep_if(&block)
    size == n ? nil : self
  end
  alias filter! select!

  # Classifies the elements of the set by the result of the given block.
  #
  # @yield [Object] Each element in the set
  # @yieldreturn [Object] The classification key
  # @return [Hash] A hash mapping classification keys to sets of elements
  def classify
    return to_enum :classify unless block_given?
    h = {}
    each { |i|
      x = yield(i)
      (h[x] ||= self.class.new).add(i)
    }
    h
  end

  # Divides the set into subsets based on the result of the given block.
  #
  # @yield [Object] Each element in the set
  # @yieldreturn [Object] The division key
  # @return [Set] A set containing the divided subsets
  def divide(&func)
    return to_enum :divide unless block_given?

    if func.arity == 2
      raise NotImplementedError, "Set#divide with 2 arity block is not implemented."
    end

    Set.new(classify(&func).values)
  end
end
