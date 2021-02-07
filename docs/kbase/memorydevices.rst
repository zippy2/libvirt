==============
Memory devices
==============

.. contents::

Basics
======

Memory devices can be divided into two families: DIMMs and NVDIMMs. The former
is typical RAM memory: it's volatile and thus its contents doesn't survive
reboots nor guest shut downs and power ons. The latter retains its contents
across reboots or power outages.

In Libvirt, there are two models for DIMMs:

* ``dimm`` model:

  ::

    <memory model='dimm'>
      <target>
        <size unit='KiB'>523264</size>
        <node>0</node>
      </target>
      <address type='dimm' slot='0'/>
    </memory>

* ``virtio-mem`` model:

  ::

    <memory model='virtio-mem'>
      <target>
        <size unit='KiB'>1048576</size>
        <node>0</node>
        <block unit='KiB'>2048</block>
        <requested unit='KiB'>524288</requested>
      </target>
      <address type='pci' domain='0x0000' bus='0x00' slot='0x02' function='0x0'/>
    </memory>

Then there are two models for NVDIMMs:

* ``nvidmm`` model:

  ::

    <memory model='nvdimm'>
      <source>
        <path>/tmp/nvdimm</path>
      </source>
      <target>
        <size unit='KiB'>523264</size>
        <node>0</node>
      </target>
      <address type='dimm' slot='0'/>
    </memory>

* ``virtio-pmem`` model:

  ::

    <memory model='virtio-pmem' access='shared'>
      <source>
        <path>/tmp/virtio_pmem</path>
      </source>
      <target>
        <size unit='KiB'>524288</size>
      </target>
      <address type='pci' domain='0x0000' bus='0x00' slot='0x05' function='0x0'/>
    </memory>


Please not that (maybe somewhat surprisingly) virtio models go onto PCI bus
instead of DIMM slots.

Furthermore, DIMMs can have ``<source/>`` element which configures backend for
devices. For NVDIMMs the element is mandatory and reflects where the contents
is saved.

See https://libvirt.org/formatdomain.html#elementsMemory

``virtio-mem`` model
====================

The ``virtio-mem`` model can be viewed as revised memory balloon. It offers
memory hotplug and hotunplug solution (without the actual hotplug of the
device). It solves problems that memory balloon can't solve on its own and thus
is more flexible than DIMM + balloon solution. ``virtio-mem`` is NUMA aware,
and thus memory can be inflated/deflated only for a subset of guest NUMA nodes.
Also, it works with chunks that are either exposed to guest or taken back from
it.

See https://virtio-mem.gitlab.io/

Under the hood, ``virtio-mem`` device is split into chunks of equal size which
are then exposed to the guest. Either all of them or only a portion depending
on user's request. Therefore there are three important sizes for
``virtio-mem``. All are to be found under ``<target/>`` element:

#. The maximum size the device can ever offer, exposed under ``<size/>``
#. The size a single block, exposed under ``<block/>``
#. The current size exposed to the guest, exposed under ``<requested/>``

For instance, the following example the maximum size is 4GiB, the block size is
2MiB and only 1GiB should be exposed to the guest:

  ::

    <memory model='virtio-mem'>
      <target>
        <size unit='KiB'>4194304</size>
        <block unit='KiB'>2048</block>
        <requested unit='KiB'>1048576</requested>
      </target>
    </memory>

Please note that ``<requested/>`` must be an integer multiple of ``<block/>``
size or zero (memory completely deflated) and has to be less or equal to
``<size/>`` (memory completely inflated). Furthermore, QEMU recommends the
``<block/>`` size to be as big as a Transparent Huge Page (usually 2MiB).

To change the size exposed to the guest, users should pass memory device XML
with nothing but ``<requested/>`` changed into the
``virDomainUpdateDeviceFlags()`` API. For user's convenience this can be done
via virsh too:

 ::

   # virsh update-memory-device $dom --requested size 2GiB

If there are two or more ``<memory/>`` devices then ``--alias`` shall be used
to tell virsh which memory device should be updated.

For running guests there is fourth size that can be found under ``<target/>``:

  ::

    <actual unit='KiB'>2097152</actual>

The ``<actual/>`` reflects the actual size consumed by the guest. In general it
can differ from ``<requested/>``. Reasons include guest kernel missing
``virtio-mem`` module and thus being unable to take offered memory, or guest
kernel being unable to free memory and allow deflation.  Since ``<actual/>``
only reports size to users, the element is never parsed. It is formatted only
into live XML.

Since changing actual allocation requires cooperation with guest kernel,
requests for change are not instant. Therefore, libvirt emits
``VIR_DOMAIN_EVENT_ID_MEMORY_DEVICE_SIZE_CHANGE`` event whenever actual
allocation changed.

Please not that using ``virtio-mem`` with memory balloon is not possible,
currently. The real reason is that libvirt's memory accounting isn't ready and
mixing these two would be confusing to users. Libvirt exposes current value of
memory balloon under ``<currentMemory/>`` but if it were to account for
``<actual/>`` too then it would be impossible to learn true size of the
balloon. Also it might result in mistakenly trying to deflate ``virtio-mem``
via ``setmem`` command.
